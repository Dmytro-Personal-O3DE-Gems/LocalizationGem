r"""Checks the localization of an O3DE project: string tables, languages.csv and the keys used
in prefabs and C++ code (LOCALIZATION_KEY).

Run it with the Python of the engine. python.cmd needs cmake in PATH, so use
"Developer PowerShell for VS 2022":
    D:\O3DE\26.05\python\python.cmd check_localization.py D:\O3DE\Projects\TestProject

Exit code: 0 = no problems, 1 = problems found, 2 = wrong arguments (argparse does that by itself).
"""

import argparse
import csv
import io
import sys
from dataclasses import dataclass, field
from pathlib import Path

# Where the string tables live inside the project. The game reads the same files from the cache
# ("@products@/assets/strings/<language>.csv"); the script reads the sources.
STRINGS_FOLDER = Path("Assets") / "Strings"

# Same as FallbackLanguage in LocalizationGemSystemComponent.cpp.
FALLBACK_LANGUAGE = "en"


@dataclass
class StringTable:
    """One language's string table, as the game would load it."""

    # key -> text. Only the rows that were read without problems.
    strings: dict[str, str] = field(default_factory=dict)

    # key -> 1-based line where the key is defined (for messages like "unused key, en.csv:5").
    key_lines: dict[str, int] = field(default_factory=dict)

    # (line, message) for everything that was skipped, sorted by line. Line 0 = the file as a whole.
    # Like the C++ parser, this does not print anything: the caller knows how loud to be.
    problems: list[tuple[int, str]] = field(default_factory=list)


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """Reads the command line. argv=None means sys.argv; a list lets other code (an Editor button) call main()."""
    parser = argparse.ArgumentParser(
        description="Checks the localization of an O3DE project: string tables, languages.csv "
        "and the keys used in prefabs and C++ code.")
    parser.add_argument("project", type=Path, help="the project folder (the one with project.json)")
    parser.add_argument("--strict", action="store_true", help="treat warnings as errors (exit code 1)")
    return parser.parse_args(argv)


def read_string_table(path: Path) -> StringTable:
    """Reads one language's CSV by the same rules as ParseStringTableCsv in LocalizationGem
    (StringTableCsvParser.h), so the script reports what the game will actually load. Keep the two in sync.

    One deliberate difference: text right after a closing quote ("abc"x) is invalid CSV.
    The game quietly loads it as abcx; the script reports the row as broken.
    """
    table = StringTable()

    try:
        data = path.read_bytes()
    except OSError as error:
        table.problems.append((0, f"cannot read the file: {error.strerror}"))
        return table

    # 1. Encoding. The whole file must be UTF-8, otherwise nothing is loaded: in a file saved
    #    as ANSI (Excel's plain "CSV") every non-English letter would be garbage.
    try:
        text = data.decode("utf-8")
    except UnicodeDecodeError as error:
        bad_line = data.count(b"\n", 0, error.start) + 1
        table.problems.append((bad_line, "the file is not valid UTF-8, nothing loaded. Save it as 'CSV UTF-8' "
                                         "(in LibreOffice: character set 'Unicode (UTF-8)')"))
        return table

    # 2. BOM: Excel's "CSV UTF-8" starts the file with EF BB BF, which decodes to the character U+FEFF.
    #    Left in place, the first column would be named "\ufeffkey" instead of "key".
    text = text.removeprefix("\ufeff")

    # 3. Text -> rows. newline="" leaves line breaks to the csv module, so a line break inside quotes works.
    #    strict=True turns broken quotes into an error; without it an unclosed quote would silently
    #    glue the rest of the file into one field.
    reader = csv.reader(io.StringIO(text, newline=""), delimiter=_detect_separator(text), strict=True)
    rows: list[tuple[int, list[str]]] = []  # (first line of the row, fields)
    while True:
        # line_num = how many lines the reader has consumed so far, so the next row starts on the line after.
        # A quoted field may span several lines; messages point at the first one, like the game.
        first_line = reader.line_num + 1
        try:
            fields = next(reader)
        except StopIteration:
            break
        except csv.Error as error:
            if "end of data" in str(error):
                # The reader ran to the end of the file looking for the closing quote.
                table.problems.append((first_line, "a quote opened on this line is never closed, "
                                                   "this row and everything after it skipped"))
            else:
                table.problems.append((first_line, f"broken quotes ({error}), row skipped"))
            continue
        rows.append((first_line, fields))  # an empty line gives fields == []

    # 4. Header: the first row that is not empty. Columns are found by name, case-insensitive,
    #    so their order does not matter.
    header_index = 0
    while header_index < len(rows) and _all_fields_empty(rows[header_index][1]):
        header_index += 1
    if header_index == len(rows):
        table.problems.append((1, "the file has no header row, nothing loaded"))
        table.problems.sort()
        return table

    header_line, header = rows[header_index]
    names = [name.strip(" \t").lower() for name in header]
    if "key" not in names or "text" not in names:
        table.problems.append((header_line, "the header row must have the columns 'key' and 'text' "
                                            "(e.g. 'key,text,comment'), nothing loaded"))
        table.problems.sort()
        return table
    key_column = names.index("key")  # index() returns the first match, like the game
    text_column = names.index("text")

    # 5. Data rows. A bad row is skipped and reported; the rest of the file is still loaded.
    for line, fields in rows[header_index + 1:]:
        if _all_fields_empty(fields):
            continue  # an empty row, also ",," that Excel writes for an empty line: not a problem

        key = fields[key_column].strip(" \t") if key_column < len(fields) else ""
        if not key:
            table.problems.append((line, "empty key, row skipped"))
            continue
        if text_column >= len(fields):
            table.problems.append((line, f"no 'text' column for key '{key}', row skipped"))
            continue

        text = fields[text_column]  # kept exactly as written: leading spaces may be intentional
        if not text:
            table.problems.append((line, f"empty text for key '{key}', row skipped"))
            continue
        if key in table.key_lines:
            table.problems.append(
                (line, f"duplicate key '{key}' (first defined on line {table.key_lines[key]}), row skipped"))
            continue

        table.strings[key] = text
        table.key_lines[key] = line

    # Quote problems were found in step 3, row problems in step 5: put them all in file order.
    table.problems.sort()
    return table


def _detect_separator(text: str) -> str:
    """',' or ';', whichever is more frequent in the first non-empty row (outside quotes).
    Excel with Spanish, Ukrainian, Russian... regional settings saves with ';'. Same as DetectSeparator in C++."""
    commas = 0
    semicolons = 0
    in_quotes = False
    saw_content = False
    for char in text:
        if char in "\r\n" and not in_quotes:
            if saw_content:
                break  # end of the first non-empty row
            continue  # skip empty rows before the header
        saw_content = True
        if char == '"':
            in_quotes = not in_quotes
        elif not in_quotes and char == ",":
            commas += 1
        elif not in_quotes and char == ";":
            semicolons += 1
    return ";" if semicolons > commas else ","


def _all_fields_empty(fields: list[str]) -> bool:
    """True for an empty line ([]) and for a row of separators only (",," -> ['', '', ''])."""
    return all(not value.strip(" \t") for value in fields)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)

    project = args.project.resolve()
    if not (project / "project.json").is_file():
        print(f"error: {project} is not an O3DE project folder (no project.json)", file=sys.stderr)
        return 1

    # For now: read the fallback table and show what came out of it. The real checks come next.
    path = project / STRINGS_FOLDER / f"{FALLBACK_LANGUAGE}.csv"
    table = read_string_table(path)
    for key, text in table.strings.items():
        print(f"{key} = {text}")
    for line, message in table.problems:
        print(f"{path}:{line}: {message}", file=sys.stderr)

    return 1 if table.problems else 0


if __name__ == "__main__":
    sys.exit(main())
