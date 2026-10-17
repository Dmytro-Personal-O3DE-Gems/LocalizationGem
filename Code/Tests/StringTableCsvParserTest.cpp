
#include <StringTableCsvParser.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

namespace LocalizationGemTests
{
    using LocalizationGem::ParseStringTableCsv;
    using LocalizationGem::StringTableParseResult;

    // LeakDetectionFixture fails a test that leaves AZ allocations behind. The parser owns nothing
    // after it returns, so every test here doubles as a leak check.
    class StringTableCsvParserTest : public UnitTest::LeakDetectionFixture
    {
    protected:
        //! The text for a key, or "<missing>" so a failed lookup shows up readably in the test output.
        static AZStd::string TextOf(const StringTableParseResult& result, const char* key)
        {
            const auto it = result.m_strings.find(key);
            return it != result.m_strings.end() ? it->second : AZStd::string("<missing>");
        }
    };

    // ---------------------------------------------------------------- the normal case

    TEST_F(StringTableCsvParserTest, ReadsSimpleTable)
    {
        const auto result = ParseStringTableCsv(
            "key,text,comment\n"
            "door.open,Open,Prompt on a closed door\n"
            "door.close,Close,Prompt on an open door\n");

        EXPECT_TRUE(result.m_problems.empty());
        ASSERT_EQ(result.m_strings.size(), 2u);
        EXPECT_EQ(TextOf(result, "door.open"), "Open");
        EXPECT_EQ(TextOf(result, "door.close"), "Close");
    }

    TEST_F(StringTableCsvParserTest, LastRowWithoutLineBreakIsRead)
    {
        const auto result = ParseStringTableCsv("key,text\ndoor.open,Open");

        EXPECT_TRUE(result.m_problems.empty());
        EXPECT_EQ(TextOf(result, "door.open"), "Open");
    }

    // ---------------------------------------------------------------- header

    TEST_F(StringTableCsvParserTest, ColumnsAreFoundByNameInAnyOrder)
    {
        const auto result = ParseStringTableCsv(
            "comment,text,key\n"
            "Prompt on a closed door,Open,door.open\n");

        EXPECT_TRUE(result.m_problems.empty());
        EXPECT_EQ(TextOf(result, "door.open"), "Open");
    }

    TEST_F(StringTableCsvParserTest, HeaderNamesIgnoreCaseAndSpaces)
    {
        const auto result = ParseStringTableCsv(" Key , TEXT \ndoor.open,Open\n");

        EXPECT_TRUE(result.m_problems.empty());
        EXPECT_EQ(TextOf(result, "door.open"), "Open");
    }

    TEST_F(StringTableCsvParserTest, CommentColumnIsOptional)
    {
        const auto result = ParseStringTableCsv("key,text\ndoor.open,Open\n");

        EXPECT_TRUE(result.m_problems.empty());
        EXPECT_EQ(TextOf(result, "door.open"), "Open");
    }

    TEST_F(StringTableCsvParserTest, HeaderWithoutTextColumnLoadsNothing)
    {
        const auto result = ParseStringTableCsv("id,value\ndoor.open,Open\n");

        EXPECT_TRUE(result.m_strings.empty());
        ASSERT_EQ(result.m_problems.size(), 1u);
        EXPECT_EQ(result.m_problems[0].m_line, 1);
    }

    TEST_F(StringTableCsvParserTest, EmptyTextReportsMissingHeader)
    {
        const auto result = ParseStringTableCsv("");

        EXPECT_TRUE(result.m_strings.empty());
        EXPECT_EQ(result.m_problems.size(), 1u);
    }

    // ---------------------------------------------------------------- what Excel and LibreOffice produce

    TEST_F(StringTableCsvParserTest, Utf8BomIsSkipped)
    {
        // Excel "CSV UTF-8" starts the file with EF BB BF.
        const auto result = ParseStringTableCsv("\xEF\xBB\xBFkey,text\ndoor.open,Open\n");

        EXPECT_TRUE(result.m_problems.empty());
        EXPECT_EQ(TextOf(result, "door.open"), "Open");
    }

    TEST_F(StringTableCsvParserTest, SemicolonSeparatorIsDetected)
    {
        // Excel with Spanish regional settings separates with ';', so ',' is plain text there.
        const auto result = ParseStringTableCsv("key;text;comment\ndoor.open;Abrir, por favor;\n");

        EXPECT_TRUE(result.m_problems.empty());
        EXPECT_EQ(TextOf(result, "door.open"), "Abrir, por favor");
    }

    TEST_F(StringTableCsvParserTest, QuotedFieldMayContainSeparator)
    {
        const auto result = ParseStringTableCsv("key,text\ndoor.locked,\"It's locked, try the key\"\n");

        EXPECT_TRUE(result.m_problems.empty());
        EXPECT_EQ(TextOf(result, "door.locked"), "It's locked, try the key");
    }

    TEST_F(StringTableCsvParserTest, DoubledQuoteInsideQuotesIsOneQuote)
    {
        const auto result = ParseStringTableCsv("key,text\nnote.run,\"He said \"\"run\"\"\"\n");

        EXPECT_TRUE(result.m_problems.empty());
        EXPECT_EQ(TextOf(result, "note.run"), "He said \"run\"");
    }

    TEST_F(StringTableCsvParserTest, LineBreakInsideQuotesIsKeptAndCounted)
    {
        // Alt+Enter in an Excel cell. Stored as LF even when the file uses CRLF.
        const auto result = ParseStringTableCsv(
            "key,text\r\n"
            "note.two_lines,\"First line\r\nSecond line\"\r\n"
            ",no key\r\n"); // line 4: reported with the right number despite the multi-line cell

        EXPECT_EQ(TextOf(result, "note.two_lines"), "First line\nSecond line");
        ASSERT_EQ(result.m_problems.size(), 1u);
        EXPECT_EQ(result.m_problems[0].m_line, 4);
    }

    TEST_F(StringTableCsvParserTest, CrlfAndLfLineEndingsBothWork)
    {
        const auto result = ParseStringTableCsv("key,text\r\ndoor.open,Open\r\ndoor.close,Close\n");

        EXPECT_TRUE(result.m_problems.empty());
        EXPECT_EQ(TextOf(result, "door.open"), "Open");
        EXPECT_EQ(TextOf(result, "door.close"), "Close");
    }

    TEST_F(StringTableCsvParserTest, EmptyRowsAreSkippedSilently)
    {
        // ",," is what Excel writes for an empty row of a three-column sheet.
        const auto result = ParseStringTableCsv("\nkey,text,comment\n\n,,\ndoor.open,Open,\n\n");

        EXPECT_TRUE(result.m_problems.empty());
        EXPECT_EQ(result.m_strings.size(), 1u);
    }

    TEST_F(StringTableCsvParserTest, KeyIsTrimmedButTextIsKeptAsIs)
    {
        const auto result = ParseStringTableCsv("key,text\n  door.open  ,  Open \n");

        EXPECT_TRUE(result.m_problems.empty());
        EXPECT_EQ(TextOf(result, "door.open"), "  Open ");
    }

    TEST_F(StringTableCsvParserTest, ExtraColumnsAreIgnored)
    {
        const auto result = ParseStringTableCsv("key,text\ndoor.open,Open,unexpected,columns\n");

        EXPECT_TRUE(result.m_problems.empty());
        EXPECT_EQ(TextOf(result, "door.open"), "Open");
    }

    TEST_F(StringTableCsvParserTest, Utf8TextIsAccepted)
    {
        // "Vidkryty" (Ukrainian "Open") written as UTF-8 bytes, so the test does not depend on the source file encoding.
        const auto result = ParseStringTableCsv("key,text\ndoor.open,\xD0\x92\xD1\x96\xD0\xB4\xD0\xBA\xD1\x80\xD0\xB8\xD1\x82\xD0\xB8\n");

        EXPECT_TRUE(result.m_problems.empty());
        EXPECT_EQ(TextOf(result, "door.open"), "\xD0\x92\xD1\x96\xD0\xB4\xD0\xBA\xD1\x80\xD0\xB8\xD1\x82\xD0\xB8");
    }

    TEST_F(StringTableCsvParserTest, NonUtf8FileLoadsNothing)
    {
        // "Atras" with an accent in Windows-1252 (Excel's plain "CSV"): the single byte E1 is not valid UTF-8.
        const auto result = ParseStringTableCsv("key,text\ndoor.open,Open\nmenu.back,Atr\xE1s\n");

        EXPECT_TRUE(result.m_strings.empty());
        ASSERT_EQ(result.m_problems.size(), 1u);
        EXPECT_EQ(result.m_problems[0].m_line, 3);
    }

    TEST_F(StringTableCsvParserTest, CyrillicInWindows1251IsRejected)
    {
        const auto result = ParseStringTableCsv("key,text\ndoor.open,\xC2\xB3\xE4\xEA\xF0\xE8\xF2\xE8\n");

        EXPECT_TRUE(result.m_strings.empty());
        EXPECT_EQ(result.m_problems.size(), 1u);
    }

    // ---------------------------------------------------------------- bad rows: skipped, reported, the rest still loads

    TEST_F(StringTableCsvParserTest, EmptyKeyIsReported)
    {
        const auto result = ParseStringTableCsv("key,text\n,Orphan text\ndoor.open,Open\n");

        EXPECT_EQ(TextOf(result, "door.open"), "Open");
        ASSERT_EQ(result.m_problems.size(), 1u);
        EXPECT_EQ(result.m_problems[0].m_line, 2);
    }

    TEST_F(StringTableCsvParserTest, EmptyTextIsReported)
    {
        const auto result = ParseStringTableCsv("key,text\ndoor.open,\ndoor.close,Close\n");

        EXPECT_EQ(result.m_strings.count("door.open"), 0u);
        EXPECT_EQ(TextOf(result, "door.close"), "Close");
        ASSERT_EQ(result.m_problems.size(), 1u);
        EXPECT_EQ(result.m_problems[0].m_line, 2);
    }

    TEST_F(StringTableCsvParserTest, MissingTextColumnIsReported)
    {
        const auto result = ParseStringTableCsv("key,text\ndoor.open\n");

        EXPECT_TRUE(result.m_strings.empty());
        ASSERT_EQ(result.m_problems.size(), 1u);
        EXPECT_EQ(result.m_problems[0].m_line, 2);
    }

    TEST_F(StringTableCsvParserTest, DuplicateKeyKeepsTheFirst)
    {
        const auto result = ParseStringTableCsv("key,text\ndoor.open,Open\ndoor.close,Close\ndoor.open,Abrir\n");

        EXPECT_EQ(TextOf(result, "door.open"), "Open");
        ASSERT_EQ(result.m_problems.size(), 1u);
        EXPECT_EQ(result.m_problems[0].m_line, 4);
    }

    TEST_F(StringTableCsvParserTest, UnclosedQuoteKeepsRowsBeforeIt)
    {
        const auto result = ParseStringTableCsv(
            "key,text\n"
            "door.open,Open\n"
            "door.close,\"Close\n"   // the quote is never closed...
            "door.lock,Lock\n");     // ...so this row is swallowed into it

        EXPECT_EQ(TextOf(result, "door.open"), "Open");
        EXPECT_EQ(result.m_strings.size(), 1u);
        ASSERT_EQ(result.m_problems.size(), 1u);
        EXPECT_EQ(result.m_problems[0].m_line, 3);
    }

} // namespace LocalizationGemTests
