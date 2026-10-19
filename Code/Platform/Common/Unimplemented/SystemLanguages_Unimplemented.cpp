
#include <SystemLanguages.h>

// Used by the platforms that do not ask the system yet: Mac, iOS, Android
// (see Code/Platform/<platform>/localizationgem_private_files.cmake).
// The game then starts in the fallback language (English).
//
// When a real version is needed, the place to ask is:
//   Mac, iOS - CFLocaleCopyPreferredLanguages() from CoreFoundation (an array like "uk-UA", "en-US");
//   Android  - java.util.Locale (LocaleList.getDefault() on Android 7+), through JNI.

namespace LocalizationGem
{
    AZStd::vector<AZStd::string> GetSystemLanguages()
    {
        return {};
    }

} // namespace LocalizationGem
