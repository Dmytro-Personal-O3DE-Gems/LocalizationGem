
#include <SystemLanguages.h>

#include <AzCore/StringFunc/StringFunc.h>

namespace LocalizationGem
{
    namespace
    {
        //! The code from the menu that equals this language tag (ignoring case), or nullptr.
        const AZStd::string* FindInMenu(AZStd::string_view tag, const AZStd::vector<LanguageInfo>& availableLanguages)
        {
            for (const LanguageInfo& language : availableLanguages)
            {
                if (AZ::StringFunc::Equal(language.m_code, tag))
                {
                    return &language.m_code;
                }
            }
            return nullptr;
        }
    } // namespace

    AZStd::string PickStartLanguage(
        const AZStd::vector<AZStd::string>& systemLanguages,
        const AZStd::vector<LanguageInfo>& availableLanguages,
        const AZStd::string& fallbackLanguage)
    {
        // The player's order matters: with "ca-ES, es-ES" and no Catalan in the menu, Spanish is the
        // right choice, so each system language is tried completely before moving on to the next one.
        for (const AZStd::string& systemLanguage : systemLanguages)
        {
            if (const AZStd::string* code = FindInMenu(systemLanguage, availableLanguages))
            {
                return *code;
            }

            // "es-ES" -> "es". The first part of a language tag is the language itself; the rest is
            // the region or the script, which a menu of plain language codes does not distinguish.
            const AZStd::string_view tag(systemLanguage);
            if (const size_t dash = tag.find('-'); dash != AZStd::string_view::npos)
            {
                if (const AZStd::string* code = FindInMenu(tag.substr(0, dash), availableLanguages))
                {
                    return *code;
                }
            }
        }
        return fallbackLanguage;
    }

} // namespace LocalizationGem
