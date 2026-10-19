r"""Checks the localization of an O3DE project: string tables, languages.csv and the keys used
in prefabs and C++ code (LOCALIZATION_KEY).

Run it with the Python of the engine. python.cmd needs cmake in PATH, so use
"Developer PowerShell for VS 2022":
    D:\O3DE\26.05\python\python.cmd check_localization.py D:\O3DE\Projects\TestProject [--strict]

Errors: a key used in the project but missing in en.csv, broken rows in a string table,
problems in languages.csv. Warnings: missing translations, keys nobody uses, a text without a key.

Exit code: 0 = no errors (with --strict: no warnings either), 1 = otherwise,
2 = wrong arguments (argparse does that by itself).
"""

import argparse
import csv
import io
import json
import os
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

# Where the string tables live inside the project. The game reads the same files from the cache
# ("@products@/assets/strings/<language>.csv"); the script reads the sources.
STRINGS_FOLDER = Path("Assets") / "Strings"

# The list of languages for the menu (key = language code, text = its name in that language).
LANGUAGE_LIST_NAME = "languages"

# Same as FallbackLanguage in LocalizationGemSystemComponent.cpp.
FALLBACK_LANGUAGE = "en"

# Project-root folders that are not sources: the asset cache, build output, per-user settings,
# and _Gems (junctions to the gems: os.walk in Python 3.10 goes through junctions, so the gems
# would be scanned as part of the project).
SKIPPED_FOLDERS = {"Cache", "build", "user", "_Gems"}

CODE_SUFFIXES = {".cpp", ".h", ".hpp", ".inl"}
PREFAB_SUFFIXES = {".prefab"}

# Every place where the marker starts ...
MARKER_START = re.compile(r"\bLOCALIZATION_KEY\s*\(")
# ... and the only form the script can read: one string literal in quotes.
MARKER_WITH_KEY = re.compile(r'LOCALIZATION_KEY\s*\(\s*"([^"\\\n]*)"\s*\)')

# C++ comments, and string / character literals: the literals are matched only so that
# "//" or "/*" inside them is not taken for a comment. Raw strings (R"(...)") are not supported.
COMMENT_OR_LITERAL = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'', re.DOTALL)

ERROR = "error"
WARNING = "warning"


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


@dataclass
class Finding:
    """One line of the report."""

    severity: str  # ERROR or WARNING
    location: str  # "Assets\Strings\en.csv:3", "Gem\Source\DoorComponent.cpp:106", "...prefab: entity 'Door', PressLabel"
    message: str


@dataclass
class KeyUse:
    """A place in the project that asks for a key."""

    key: str
    location: str


@dataclass
class Report:
    """Everything check() found, plus a few numbers for the summary line."""

    languages: list[str] = field(default_factory=list)
    defined_keys: int = 0
    used_keys: int = 0
    findings: list[Finding] = field(default_factory=list)


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


def read_languages(project: Path) -> tuple[list[str], list[Finding]]:
    """Reads languages.csv the way LoadAvailableLanguages does and returns the codes the game will offer.

    The game: no list -> English only; a listed language without its file is left out of the menu.
    The script reports both, plus a language file that exists but is not listed (nobody can choose it).
    """
    strings_folder = project / STRINGS_FOLDER
    path = strings_folder / f"{LANGUAGE_LIST_NAME}.csv"
    findings: list[Finding] = []

    if not path.is_file():
        findings.append(Finding(ERROR, _location(project, path),
                                "the list of languages is missing: the game offers English only"))
        return [FALLBACK_LANGUAGE], findings

    table = read_string_table(path)
    for line, message in table.problems:
        findings.append(Finding(ERROR, _location(project, path, line), message))

    if FALLBACK_LANGUAGE not in table.strings:
        findings.append(Finding(ERROR, _location(project, path),
                                f"'{FALLBACK_LANGUAGE}' is not listed: the fallback language must always be in the menu"))

    languages: list[str] = []
    for code in table.strings:
        if (strings_folder / f"{code}.csv").is_file():
            languages.append(code)
        else:
            findings.append(Finding(ERROR, _location(project, path, table.key_lines[code]),
                                    f"language '{code}' is listed, but {code}.csv does not exist: "
                                    "the game leaves it out of the menu"))

    listed = {code.lower() for code in table.strings}
    for csv_path in sorted(strings_folder.glob("*.csv")):
        name = csv_path.stem.lower()
        if name != LANGUAGE_LIST_NAME and name not in listed:
            findings.append(Finding(WARNING, _location(project, csv_path),
                                    f"not listed in {LANGUAGE_LIST_NAME}.csv: players cannot choose this language"))

    return languages, findings


def collect_code_keys(project: Path) -> tuple[list[KeyUse], list[Finding]]:
    """Finds LOCALIZATION_KEY("...") in the project's C++ files. Comments are skipped: a marker
    inside a comment does not exist for the compiler, so it must not count as a use."""
    uses: list[KeyUse] = []
    findings: list[Finding] = []

    for path in _project_files(project, CODE_SUFFIXES):
        # Only the ASCII marker matters, so a file in another encoding must not stop the check.
        text = _blank_comments(path.read_text(encoding="utf-8", errors="replace"))

        for start in MARKER_START.finditer(text):
            line = text.count("\n", 0, start.start()) + 1
            line_start = text.rfind("\n", 0, start.start()) + 1
            if text[line_start:start.start()].lstrip().startswith("#define"):
                continue  # the definition of the macro itself, not a use

            location = _location(project, path, line)
            marker = MARKER_WITH_KEY.match(text, start.start())
            if marker is None:
                # Compiles if what is inside becomes a string literal after substitution
                # ("door." "open", or another macro), but the script cannot see the key.
                findings.append(Finding(ERROR, location, "LOCALIZATION_KEY without a single string literal "
                                                         "in quotes: the script cannot read this key"))
            elif not marker.group(1).strip():
                findings.append(Finding(ERROR, location, "LOCALIZATION_KEY with an empty key"))
            else:
                uses.append(KeyUse(marker.group(1).strip(), location))

    return uses, findings


def collect_prefab_keys(project: Path) -> tuple[list[KeyUse], list[Finding]]:
    """Finds the "key + default text" pairs (InteractionSystem::LocalizedText) in the project's prefabs.

    In a .prefab a pair is a JSON object with the fields "Key" and "DefaultText". A field with its
    default value (an empty string) is not written at all, so a pair may have only one of them,
    and an empty pair is not written at all.
    """
    uses: list[KeyUse] = []
    findings: list[Finding] = []

    for path in _project_files(project, PREFAB_SUFFIXES):
        file_location = _location(project, path)
        try:
            with open(path, encoding="utf-8") as file:
                document = json.load(file)
        except (OSError, ValueError) as error:  # ValueError covers broken JSON and broken UTF-8
            findings.append(Finding(ERROR, file_location, f"cannot read the prefab: {error}"))
            continue

        # A function inside a function: like a C++ lambda with [&], it sees uses, findings and file_location.
        def walk(node: object, where: str, field_name: str) -> None:
            if isinstance(node, list):
                for item in node:
                    walk(item, where, field_name)
                return
            if not isinstance(node, dict):
                return  # a number, a string, true/false/null: nothing inside

            if _is_localized_text(node):
                key = node.get("Key", "").strip()
                text_location = f"{where}, {field_name}"
                if key:
                    uses.append(KeyUse(key, text_location))
                elif node.get("DefaultText"):
                    findings.append(Finding(WARNING, text_location,
                                            f"text '{node['DefaultText']}' has no key: it is never translated"))
                return

            # A change made to a nested prefab in this prefab: {"op": "replace", "path": ".../PressLabel/Key", "value": "door.open"}.
            # A whole pair as "value" is found by the walk below; a lone key is caught here.
            path_text = node.get("path")
            value = node.get("value")
            if isinstance(path_text, str) and path_text.endswith("/Key") and isinstance(value, str) and value.strip():
                uses.append(KeyUse(value.strip(), f"{where}, override {path_text}"))

            # Name the place as precisely as possible: an entity, or a nested prefab.
            if isinstance(node.get("Name"), str) and "Components" in node:
                where = f"{file_location}: entity '{node['Name']}'"
            elif isinstance(node.get("Source"), str) and "Patches" in node:
                where = f"{file_location}: instance of '{node['Source']}'"

            for name, child in node.items():
                # The "value" of a change is better named by its path (".../InteractionInfo/PressLabel").
                child_name = path_text if name == "value" and isinstance(path_text, str) else name
                walk(child, where, child_name)

        walk(document, file_location, "")

    return uses, findings


def check(project: Path) -> Report:
    """Reads everything and compares: languages, string tables, keys used in code and prefabs."""
    report = Report()
    strings_folder = project / STRINGS_FOLDER

    languages, findings = read_languages(project)
    report.languages = languages
    report.findings += findings

    # Every language the game will offer, read the way the game reads it.
    tables: dict[str, StringTable] = {}
    for code in languages:
        path = strings_folder / f"{code}.csv"
        tables[code] = read_string_table(path)
        for line, message in tables[code].problems:
            report.findings.append(Finding(ERROR, _location(project, path, line), message))

    # English is the reference: every key must be there. Read it even if languages.csv forgot it.
    fallback_path = strings_folder / f"{FALLBACK_LANGUAGE}.csv"
    if FALLBACK_LANGUAGE in tables:
        fallback = tables[FALLBACK_LANGUAGE]
    else:
        fallback = read_string_table(fallback_path)
        for line, message in fallback.problems:
            report.findings.append(Finding(ERROR, _location(project, fallback_path, line), message))
    report.defined_keys = len(fallback.strings)

    code_uses, findings = collect_code_keys(project)
    report.findings += findings
    prefab_uses, findings = collect_prefab_keys(project)
    report.findings += findings
    uses = code_uses + prefab_uses
    used_keys = {use.key for use in uses}  # a set: each key once, however many places use it
    report.used_keys = len(used_keys)

    # 1. Used, but not in English: the player sees the default text, or the key itself if there is none.
    for use in uses:
        if use.key not in fallback.strings:
            report.findings.append(Finding(ERROR, use.location, f"key '{use.key}' is not in {FALLBACK_LANGUAGE}.csv"))

    # 2. In English, but nobody uses it: an old key, or a typo on one side.
    for key, line in fallback.key_lines.items():
        if key not in used_keys:
            report.findings.append(Finding(WARNING, _location(project, fallback_path, line),
                                           f"key '{key}' is not used in the project's prefabs or code"))

    # 3. Other languages against English.
    for code, table in tables.items():
        if code == FALLBACK_LANGUAGE:
            continue
        path = strings_folder / f"{code}.csv"

        missing = [key for key in fallback.strings if key not in table.strings]
        if missing:
            report.findings.append(Finding(WARNING, _location(project, path),
                                           f"{len(missing)} key(s) not translated, shown in English: {', '.join(missing)}"))

        for key, line in table.key_lines.items():
            if key not in fallback.strings:
                report.findings.append(Finding(WARNING, _location(project, path, line),
                                               f"key '{key}' is not in {FALLBACK_LANGUAGE}.csv: "
                                               "a typo, or a key removed from English"))

    return report


def print_report(report: Report, project: Path, strict: bool) -> int:
    """Prints the report and returns the exit code."""
    errors = [finding for finding in report.findings if finding.severity == ERROR]
    warnings = [finding for finding in report.findings if finding.severity == WARNING]

    print(f"Localization check: {project}")
    print(f"Languages: {', '.join(report.languages)} | keys in {FALLBACK_LANGUAGE}.csv: {report.defined_keys}"
          f" | keys used: {report.used_keys}")
    print()

    for finding in errors + warnings:  # errors first: they are the ones to fix
        print(f"{finding.severity.upper():<8} {finding.location}: {finding.message}")
    if report.findings:
        print()

    summary = f"{len(errors)} error(s), {len(warnings)} warning(s)"
    if strict and warnings:
        summary += " (--strict: warnings count as errors)"
    print(summary)

    failed = bool(errors) or (strict and bool(warnings))
    return 1 if failed else 0


def main(argv: list[str] | None = None) -> int:
    # Entity names and texts may be in any language. A console shows them, but a redirected output
    # (a file, later the Editor) may use an encoding without Cyrillic: print \u0443 there instead of crashing.
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(errors="backslashreplace")

    args = parse_args(argv)

    project = args.project.resolve()
    if not (project / "project.json").is_file():
        print(f"error: {project} is not an O3DE project folder (no project.json)", file=sys.stderr)
        return 1

    return print_report(check(project), project, args.strict)


# --- Small helpers --------------------------------------------------------------------------------------


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


def _project_files(project: Path, suffixes: set[str]):
    """Yields the project's files with these extensions, in a stable order, without the skipped folders.
    'yield' makes this a generator: it hands out one file at a time instead of building a list."""
    for folder, subfolders, files in os.walk(project):
        at_root = Path(folder) == project
        # Changing the list in place ([:] =) tells os.walk not to go into the removed folders at all.
        subfolders[:] = sorted(name for name in subfolders
                               if not name.startswith(".") and not (at_root and name in SKIPPED_FOLDERS))
        for name in sorted(files):
            if Path(name).suffix.lower() in suffixes:
                yield Path(folder) / name


def _blank_comments(text: str) -> str:
    """Replaces every C++ comment with spaces, keeping the line breaks, so line numbers stay right."""
    def replace(match: re.Match) -> str:
        found = match.group(0)
        if found.startswith("/"):
            return re.sub(r"[^\n]", " ", found)
        return found  # a string or character literal: kept as it is
    return COMMENT_OR_LITERAL.sub(replace, text)


def _is_localized_text(node: dict) -> bool:
    """A JSON object that has only "Key" and/or "DefaultText", both strings."""
    return bool(node) and set(node) <= {"Key", "DefaultText"} and all(isinstance(v, str) for v in node.values())


def _location(project: Path, path: Path, line: int = 0) -> str:
    """'Assets\\Strings\\en.csv:3', or without ':line' for the file as a whole."""
    relative = path.relative_to(project)
    return f"{relative}:{line}" if line else str(relative)


if __name__ == "__main__":
    sys.exit(main())
