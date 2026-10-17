
#include "StringTableCsvParser.h"

namespace LocalizationGem
{
    namespace
    {
        //! One CSV row split into fields, plus the line it starts on (a quoted field may span several lines).
        struct CsvRow
        {
            int m_line = 0;
            AZStd::vector<AZStd::string> m_fields;
        };

        bool IsLineBreak(char c)
        {
            return c == '\n' || c == '\r';
        }

        //! Returns 0 if the text is valid UTF-8, otherwise the 1-based line of the first invalid byte.
        //! A CSV saved by Excel as plain "CSV" (not "CSV UTF-8") is in an ANSI code page: every non-ASCII
        //! letter (Cyrillic, Spanish accented letters) is a single byte >= 0x80 that breaks the UTF-8 rules checked here.
        int FindFirstInvalidUtf8Line(AZStd::string_view text)
        {
            int line = 1;
            size_t i = 0;
            while (i < text.size())
            {
                const unsigned char lead = static_cast<unsigned char>(text[i]);
                if (lead < 0x80)
                {
                    if (lead == '\n')
                    {
                        ++line;
                    }
                    ++i;
                    continue;
                }

                // Lead byte tells how many bytes the character has. C0, C1 and F5..FF never appear in UTF-8.
                size_t length = 0;
                if (lead >= 0xC2 && lead <= 0xDF)
                {
                    length = 2;
                }
                else if (lead >= 0xE0 && lead <= 0xEF)
                {
                    length = 3;
                }
                else if (lead >= 0xF0 && lead <= 0xF4)
                {
                    length = 4;
                }
                else
                {
                    return line;
                }

                if (i + length > text.size())
                {
                    return line; // the file ends in the middle of a character
                }

                // Every following byte must look like 10xxxxxx.
                for (size_t k = 1; k < length; ++k)
                {
                    if ((static_cast<unsigned char>(text[i + k]) & 0xC0) != 0x80)
                    {
                        return line;
                    }
                }

                // Forms that are structurally fine but forbidden: overlong encodings, UTF-16 surrogates, > U+10FFFF.
                const unsigned char second = static_cast<unsigned char>(text[i + 1]);
                if ((lead == 0xE0 && second < 0xA0) || (lead == 0xED && second > 0x9F) ||
                    (lead == 0xF0 && second < 0x90) || (lead == 0xF4 && second > 0x8F))
                {
                    return line;
                }

                i += length;
            }
            return 0;
        }

        //! Picks ',' or ';' by counting both in the header row (outside quotes).
        //! Excel with Spanish, Ukrainian, Russian... regional settings saves with ';',
        //! because there ',' is the decimal separator. The header is our own known text, so this is reliable.
        char DetectSeparator(AZStd::string_view text)
        {
            size_t commas = 0;
            size_t semicolons = 0;
            bool inQuotes = false;
            bool sawContent = false;
            for (const char c : text)
            {
                if (IsLineBreak(c) && !inQuotes)
                {
                    if (sawContent)
                    {
                        break; // end of the first non-empty row
                    }
                    continue; // skip empty rows before the header
                }
                sawContent = true;
                if (c == '"')
                {
                    inQuotes = !inQuotes;
                }
                else if (!inQuotes && c == ',')
                {
                    ++commas;
                }
                else if (!inQuotes && c == ';')
                {
                    ++semicolons;
                }
            }
            return semicolons > commas ? ';' : ',';
        }

        //! Splits CSV text into rows of fields. A small state machine that reads one character at a time.
        //! Returns false if a quoted field is never closed; then unclosedLine is the line where that row starts,
        //! and rows contains only the rows before it.
        bool SplitIntoRows(AZStd::string_view text, char separator, AZStd::vector<CsvRow>& rows, int& unclosedLine)
        {
            enum class State
            {
                FieldStart,     //!< nothing read for the current field yet
                Unquoted,       //!< inside a plain field
                Quoted,         //!< inside "..."
                QuoteInQuoted   //!< just read a '"' inside "...": either the closing quote or the first half of ""
            };

            State state = State::FieldStart;
            CsvRow row;
            AZStd::string field;
            bool rowHasContent = false; // true once anything (even a separator) was read for this row
            int line = 1;
            row.m_line = 1;

            auto endField = [&]()
            {
                row.m_fields.push_back(AZStd::move(field));
                field.clear();
            };
            auto endRow = [&]()
            {
                endField();
                rows.push_back(AZStd::move(row));
                row = CsvRow{};
                rowHasContent = false;
            };

            for (size_t i = 0; i < text.size(); ++i)
            {
                const char c = text[i];

                // CRLF counts as one line break. A lone CR (old Mac files) counts as one too.
                const bool isLineBreak = IsLineBreak(c);
                if (c == '\r' && i + 1 < text.size() && text[i + 1] == '\n')
                {
                    ++i;
                }

                switch (state)
                {
                case State::FieldStart:
                    if (c == '"')
                    {
                        state = State::Quoted;
                        rowHasContent = true;
                    }
                    else if (c == separator)
                    {
                        endField(); // empty field
                        rowHasContent = true;
                    }
                    else if (isLineBreak)
                    {
                        if (rowHasContent)
                        {
                            endRow();
                        }
                        else
                        {
                            row = CsvRow{}; // empty line: no row at all
                        }
                        ++line;
                        row.m_line = line;
                    }
                    else
                    {
                        field.push_back(c);
                        state = State::Unquoted;
                        rowHasContent = true;
                    }
                    break;

                case State::Unquoted:
                    if (c == separator)
                    {
                        endField();
                        state = State::FieldStart;
                    }
                    else if (isLineBreak)
                    {
                        endRow();
                        ++line;
                        row.m_line = line;
                        state = State::FieldStart;
                    }
                    else
                    {
                        field.push_back(c); // a '"' in the middle of a plain field is kept as is
                    }
                    break;

                case State::Quoted:
                    if (c == '"')
                    {
                        state = State::QuoteInQuoted;
                    }
                    else if (isLineBreak)
                    {
                        field.push_back('\n'); // a line break inside a cell (Alt+Enter in Excel), always stored as LF
                        ++line;
                    }
                    else
                    {
                        field.push_back(c);
                    }
                    break;

                case State::QuoteInQuoted:
                    if (c == '"')
                    {
                        field.push_back('"'); // "" inside quotes is one literal quote
                        state = State::Quoted;
                    }
                    else if (c == separator)
                    {
                        endField();
                        state = State::FieldStart;
                    }
                    else if (isLineBreak)
                    {
                        endRow();
                        ++line;
                        row.m_line = line;
                        state = State::FieldStart;
                    }
                    else
                    {
                        // Text right after a closing quote ("abc"x). Not valid CSV; keep it rather than lose it.
                        field.push_back(c);
                        state = State::Unquoted;
                    }
                    break;
                }
            }

            if (state == State::Quoted)
            {
                unclosedLine = row.m_line;
                return false;
            }
            if (rowHasContent)
            {
                endRow(); // the last row had no line break after it
            }
            return true;
        }

        void TrimSpaces(AZStd::string& value)
        {
            const size_t first = value.find_first_not_of(" \t");
            if (first == AZStd::string::npos)
            {
                value.clear();
                return;
            }
            const size_t last = value.find_last_not_of(" \t");
            value = value.substr(first, last - first + 1);
        }

        AZStd::string ToLowerAscii(AZStd::string value)
        {
            for (char& c : value)
            {
                if (c >= 'A' && c <= 'Z')
                {
                    c = static_cast<char>(c - 'A' + 'a');
                }
            }
            return value;
        }

        bool AllFieldsEmpty(const CsvRow& row)
        {
            for (const AZStd::string& field : row.m_fields)
            {
                if (field.find_first_not_of(" \t") != AZStd::string::npos)
                {
                    return false;
                }
            }
            return true;
        }

        void AddProblem(StringTableParseResult& result, int line, AZStd::string message)
        {
            result.m_problems.push_back({ line, AZStd::move(message) });
        }
    } // namespace

    StringTableParseResult ParseStringTableCsv(AZStd::string_view csvText)
    {
        StringTableParseResult result;

        // 1. Encoding. Checked first and for the whole text: in a non-UTF-8 file every row with
        //    a non-English letter would be garbage, so loading "the rest" makes no sense.
        if (const int badLine = FindFirstInvalidUtf8Line(csvText); badLine != 0)
        {
            AddProblem(result, badLine,
                "the file is not valid UTF-8, nothing loaded. Save it as 'CSV UTF-8' "
                "(in LibreOffice: character set 'Unicode (UTF-8)')");
            return result;
        }

        // 2. BOM: Excel's "CSV UTF-8" starts the file with EF BB BF. Without this the first column
        //    would be named "\xEF\xBB\xBFkey" and the "key" column would not be found.
        constexpr AZStd::string_view utf8Bom = "\xEF\xBB\xBF";
        if (csvText.starts_with(utf8Bom))
        {
            csvText.remove_prefix(utf8Bom.size());
        }

        // 3. Text -> rows of fields.
        AZStd::vector<CsvRow> rows;
        int unclosedLine = 0;
        const bool closed = SplitIntoRows(csvText, DetectSeparator(csvText), rows, unclosedLine);

        // 4. Header: the first row that is not empty. Columns are found by name.
        size_t rowIndex = 0;
        while (rowIndex < rows.size() && AllFieldsEmpty(rows[rowIndex]))
        {
            ++rowIndex;
        }
        if (rowIndex == rows.size())
        {
            AddProblem(result, closed ? 1 : unclosedLine,
                closed ? "the file has no header row, nothing loaded"
                       : "a quote is never closed in the header row, nothing loaded");
            return result;
        }

        const CsvRow& header = rows[rowIndex];
        constexpr size_t notFound = static_cast<size_t>(-1);
        size_t keyColumn = notFound;
        size_t textColumn = notFound;
        for (size_t column = 0; column < header.m_fields.size(); ++column)
        {
            AZStd::string name = header.m_fields[column];
            TrimSpaces(name);
            name = ToLowerAscii(AZStd::move(name));
            if (name == "key" && keyColumn == notFound)
            {
                keyColumn = column;
            }
            else if (name == "text" && textColumn == notFound)
            {
                textColumn = column;
            }
        }
        if (keyColumn == notFound || textColumn == notFound)
        {
            AddProblem(result, header.m_line,
                "the header row must have the columns 'key' and 'text' (e.g. 'key,text,comment'), nothing loaded");
            return result;
        }

        // 5. Data rows. A bad row is skipped and reported; the rest of the file is still loaded.
        AZStd::unordered_map<AZStd::string, int> firstLineOfKey;
        for (++rowIndex; rowIndex < rows.size(); ++rowIndex)
        {
            const CsvRow& row = rows[rowIndex];
            if (AllFieldsEmpty(row))
            {
                continue; // an empty row, also ",," that Excel writes for an empty line: not a problem
            }

            AZStd::string key = keyColumn < row.m_fields.size() ? row.m_fields[keyColumn] : AZStd::string();
            TrimSpaces(key);
            if (key.empty())
            {
                AddProblem(result, row.m_line, "empty key, row skipped");
                continue;
            }
            if (textColumn >= row.m_fields.size())
            {
                AddProblem(result, row.m_line, AZStd::string::format("no 'text' column for key '%s', row skipped", key.c_str()));
                continue;
            }

            // The text is kept exactly as written: leading spaces or line breaks may be intentional.
            const AZStd::string& text = row.m_fields[textColumn];
            if (text.empty())
            {
                AddProblem(result, row.m_line, AZStd::string::format("empty text for key '%s', row skipped", key.c_str()));
                continue;
            }

            if (const auto it = firstLineOfKey.find(key); it != firstLineOfKey.end())
            {
                AddProblem(result, row.m_line, AZStd::string::format(
                    "duplicate key '%s' (first defined on line %d), row skipped", key.c_str(), it->second));
                continue;
            }

            firstLineOfKey.emplace(key, row.m_line);
            result.m_strings.emplace(AZStd::move(key), text);
        }

        // 6. An unclosed quote swallows the rest of the file, so it is reported last, after the rows that did load.
        if (!closed)
        {
            AddProblem(result, unclosedLine,
                "a quote opened on this line is never closed, this row and everything after it skipped");
        }

        return result;
    }

} // namespace LocalizationGem
