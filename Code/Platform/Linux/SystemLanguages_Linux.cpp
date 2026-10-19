
#include <SystemLanguages.h>

#include <AzCore/std/algorithm.h>

#include <cstdlib>

namespace LocalizationGem
{
    namespace
    {
        //! "uk_UA.UTF-8" or "sr_RS@latin" -> "uk-UA" / "sr-RS". Empty for "C" and "POSIX" (no language set).
        AZStd::string ToLanguageTag(AZStd::string_view localeName)
        {
            localeName = localeName.substr(0, localeName.find_first_of(".@")); // drop the encoding and the modifier
            if (localeName.empty() || localeName == "C" || localeName == "POSIX")
            {
                return {};
            }

            AZStd::string tag(localeName);
            for (char& c : tag)
            {
                if (c == '_')
                {
                    c = '-';
                }
            }
            return tag;
        }

        void AddUnique(AZStd::vector<AZStd::string>& languages, AZStd::string tag)
        {
            if (!tag.empty() && AZStd::find(languages.begin(), languages.end(), tag) == languages.end())
            {
                languages.push_back(AZStd::move(tag));
            }
        }
    } // namespace

    AZStd::vector<AZStd::string> GetSystemLanguages()
    {
        // Linux has no single system call for this: programs read environment variables, like gettext does.
        // LANGUAGE is a priority list ("uk:en"); LC_ALL, LC_MESSAGES and LANG name one locale ("uk_UA.UTF-8"),
        // and the first one that is set wins.
        AZStd::vector<AZStd::string> languages;

        if (const char* list = std::getenv("LANGUAGE"))
        {
            AZStd::string_view rest(list);
            while (!rest.empty())
            {
                const size_t colon = rest.find(':');
                AddUnique(languages, ToLanguageTag(rest.substr(0, colon)));
                rest = colon == AZStd::string_view::npos ? AZStd::string_view() : rest.substr(colon + 1);
            }
        }

        for (const char* variable : { "LC_ALL", "LC_MESSAGES", "LANG" })
        {
            if (const char* value = std::getenv(variable); value && *value)
            {
                AddUnique(languages, ToLanguageTag(value));
                break;
            }
        }
        return languages;
    }

} // namespace LocalizationGem
