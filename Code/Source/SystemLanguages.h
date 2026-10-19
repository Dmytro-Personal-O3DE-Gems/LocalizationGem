
#pragma once

#include <LocalizationGem/LocalizationGemBus.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace LocalizationGem
{
    //! The languages the player chose for the operating system's interface, most preferred first,
    //! as language tags: "es-ES", "uk-UA", "en". Empty when the platform cannot tell.
    //!
    //! One implementation per platform (PAL): Code/Platform/<platform>/localizationgem_private_files.cmake
    //! lists the file that is built for that platform. Platforms without an implementation use
    //! Code/Platform/Common/Unimplemented/SystemLanguages_Unimplemented.cpp, which returns an empty list.
    AZStd::vector<AZStd::string> GetSystemLanguages();

    //! Picks the language to start the game in: the first system language that is in the menu,
    //! otherwise fallbackLanguage. For each system language an exact match is tried first ("pt-br"),
    //! then the part before the first '-' ("es-ES" -> "es"). Case is ignored; the code is returned
    //! as written in the menu.
    //!
    //! Pure function: no platform, no files, no logging, so it is unit-tested on its own.
    AZStd::string PickStartLanguage(
        const AZStd::vector<AZStd::string>& systemLanguages,
        const AZStd::vector<LanguageInfo>& availableLanguages,
        const AZStd::string& fallbackLanguage);

} // namespace LocalizationGem
