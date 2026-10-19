
#include <SystemLanguages.h>

// O3DE's way to include windows.h: it sets WIN32_LEAN_AND_MEAN, NOMINMAX and the like first.
#include <AzCore/PlatformIncl.h>
#include <AzCore/std/string/conversions.h>

namespace LocalizationGem
{
    AZStd::vector<AZStd::string> GetSystemLanguages()
    {
        // The Windows display languages (Settings > Time & language > Language), most preferred first.
        // Windows returns them in one buffer of UTF-16 strings, each ending with \0, the whole list
        // ending with one more \0: "es-ES\0en-US\0\0". The first call asks for the size, the second fills it.
        ULONG languageCount = 0;
        ULONG bufferSize = 0;
        if (!GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &languageCount, nullptr, &bufferSize) || bufferSize == 0)
        {
            return {};
        }

        AZStd::vector<wchar_t> buffer(bufferSize, L'\0');
        if (!GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &languageCount, buffer.data(), &bufferSize))
        {
            return {};
        }

        AZStd::vector<AZStd::string> languages;
        for (const wchar_t* cursor = buffer.data(); *cursor != L'\0';)
        {
            const AZStd::wstring_view name(cursor); // up to the next \0
            AZStd::string language;
            AZStd::to_string(language, name); // UTF-16 -> UTF-8
            languages.push_back(AZStd::move(language));
            cursor += name.size() + 1; // skip the name and its \0
        }
        return languages;
    }

} // namespace LocalizationGem
