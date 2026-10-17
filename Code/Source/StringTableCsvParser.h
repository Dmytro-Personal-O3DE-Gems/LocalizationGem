
#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

namespace LocalizationGem
{
    //! One thing that went wrong while reading a string table.
    //! The parser never logs by itself: the caller knows the file name and decides how loud to be,
    //! and unit tests can inspect the list.
    struct StringTableProblem
    {
        //! 1-based line in the text where the problem starts (for a multi-line row: its first line).
        int m_line = 0;

        //! Human-readable description, e.g. "duplicate key 'door.open' (first defined on line 3), row skipped".
        AZStd::string m_message;
    };

    //! What the parser produced from one CSV text.
    struct StringTableParseResult
    {
        //! Key -> text. Contains only the rows that were read without problems.
        AZStd::unordered_map<AZStd::string, AZStd::string> m_strings;

        //! Everything that was skipped and why, in the order it was found. Empty when the file is clean.
        AZStd::vector<StringTableProblem> m_problems;
    };

    //! Parses the string table of ONE language from CSV text.
    //!
    //! Expected format: the first non-empty row is a header with column names, matched by name
    //! (case-insensitive), so the order of the columns does not matter:
    //!   key,text,comment
    //!   door.open,Open,Prompt on a closed door
    //! "key" and "text" are required. "comment" (for translators) and any other column are ignored.
    //!
    //! Tolerates what Excel and LibreOffice produce: a UTF-8 BOM, ';' instead of ',' as the separator
    //! (detected from the header row), quoted fields with separators, line breaks and doubled quotes ("")
    //! inside, CRLF or LF line endings, empty rows (also rows of separators only, like ",,").
    //!
    //! Bad rows are skipped and reported in m_problems: empty key, missing or empty text, duplicate key
    //! (the first one wins). Nothing is loaded when the text is not valid UTF-8 (a CSV saved in ANSI),
    //! when the header has no "key" or "text" column, or when a quote is never closed (everything
    //! from that row on is unreliable, so the rows before it are kept and the rest is dropped).
    //!
    //! Pure function: no files, no engine, no logging. That is what makes it unit-testable.
    StringTableParseResult ParseStringTableCsv(AZStd::string_view csvText);

} // namespace LocalizationGem
