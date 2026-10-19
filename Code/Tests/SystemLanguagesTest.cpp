
#include <SystemLanguages.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

namespace LocalizationGemTests
{
    using LocalizationGem::LanguageInfo;
    using LocalizationGem::PickStartLanguage;

    class PickStartLanguageTest : public UnitTest::LeakDetectionFixture
    {
    protected:
        //! The menu as LoadAvailableLanguages builds it from languages.csv.
        static AZStd::vector<LanguageInfo> Menu()
        {
            return { { "en", "English" }, { "es", "Español" }, { "uk", "Українська" } };
        }
    };

    TEST_F(PickStartLanguageTest, ExactCodeIsPicked)
    {
        EXPECT_EQ(PickStartLanguage({ "uk" }, Menu(), "en"), "uk");
    }

    TEST_F(PickStartLanguageTest, RegionIsIgnoredWhenOnlyTheLanguageIsInTheMenu)
    {
        EXPECT_EQ(PickStartLanguage({ "es-ES" }, Menu(), "en"), "es");
        EXPECT_EQ(PickStartLanguage({ "uk-UA" }, Menu(), "en"), "uk");
    }

    TEST_F(PickStartLanguageTest, CaseIsIgnoredAndTheMenuSpellingIsReturned)
    {
        EXPECT_EQ(PickStartLanguage({ "UK-ua" }, Menu(), "en"), "uk");
    }

    TEST_F(PickStartLanguageTest, PlayerOrderWins)
    {
        // Catalan is not in the menu, Spanish is the player's second choice; English comes after it.
        EXPECT_EQ(PickStartLanguage({ "ca-ES", "es-ES", "en-US" }, Menu(), "en"), "es");
    }

    TEST_F(PickStartLanguageTest, ExactMatchBeatsTheLanguagePart)
    {
        AZStd::vector<LanguageInfo> menu = Menu();
        menu.push_back({ "pt", "Português" });
        menu.push_back({ "pt-br", "Português (Brasil)" });

        EXPECT_EQ(PickStartLanguage({ "pt-BR" }, menu, "en"), "pt-br");
        EXPECT_EQ(PickStartLanguage({ "pt-PT" }, menu, "en"), "pt");
    }

    TEST_F(PickStartLanguageTest, FallbackWhenNothingMatches)
    {
        EXPECT_EQ(PickStartLanguage({ "de-DE", "fr-FR" }, Menu(), "en"), "en");
        EXPECT_EQ(PickStartLanguage({}, Menu(), "en"), "en"); // the platform could not tell
    }

} // namespace LocalizationGemTests
