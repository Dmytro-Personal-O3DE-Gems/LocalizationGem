
#pragma once

#include <AzCore/Component/Component.h>
#include <LocalizationGem/LocalizationGemBus.h>
#include <LocalizationContract/LocalizationContractBus.h>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Console/IConsole.h>   // AZ_CONSOLEFUNC, AZ::ConsoleCommandContainer


namespace LocalizationGem
{
    class LocalizationGemSystemComponent
        : public AZ::Component
        , protected LocalizationGemRequestBus::Handler
        , public LocalizationContract::LocalizationContractRequestBus::Handler
    {
    public:
        AZ_COMPONENT_DECL(LocalizationGemSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        ////////////////////////////////////////////////////////////////////////
        // LocalizationContractRequestBus interface implementation
        AZStd::optional<AZStd::string> GetLocalizedString(const AZStd::string& key) const override;
        AZStd::string GetCurrentLanguage() const override;
        ////////////////////////////////////////////////////////////////////////

        LocalizationGemSystemComponent();
        ~LocalizationGemSystemComponent();

    protected:
        ////////////////////////////////////////////////////////////////////////
        // LocalizationGemRequestBus interface implementation
        void SetLanguage(const AZStd::string& languageCode) override;
        AZStd::vector<LanguageInfo> GetAvailableLanguages() const override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    private:
        // Debug console command. In the console: LocalizationGemSystemComponent.SetLanguageCommand uk
        // A separate name from SetLanguage on purpose: AZ_CONSOLEFUNC takes &Class::Method,
        // and two methods with the same name would make that address ambiguous.
        void SetLanguageCommand(const AZ::ConsoleCommandContainer& arguments);
        AZ_CONSOLEFUNC(LocalizationGemSystemComponent, SetLanguageCommand, AZ::ConsoleFunctorFlags::Null,
            "Debug: switches the current language. Usage: LocalizationGemSystemComponent.SetLanguageCommand <code>, e.g. uk");

        // Debug console command. In the console: LocalizationGemSystemComponent.ListLanguagesCommand
        // Prints GetAvailableLanguages() and the current language to the log.
        void ListLanguagesCommand(const AZ::ConsoleCommandContainer& arguments);
        AZ_CONSOLEFUNC(LocalizationGemSystemComponent, ListLanguagesCommand, AZ::ConsoleFunctorFlags::Null,
            "Debug: prints the languages the player can choose. Usage: LocalizationGemSystemComponent.ListLanguagesCommand");

        // Key -> text of one language.
        using StringTable = AZStd::unordered_map<AZStd::string, AZStd::string>;

        // Reads "@products@/assets/strings/<languageCode>.csv" and parses it.
        // Rows the parser skipped are reported to the log here, the caller does not see them.
        // Returns nullopt when the file is missing or cannot be read. The caller decides how
        // serious that is: an error for the fallback language, a warning for any other one.
        AZStd::optional<StringTable> LoadStringTable(const AZStd::string& languageCode) const;

        // Builds the language menu from "assets/strings/languages.csv" (key = code, text = name in that language).
        // Drops languages whose string table file does not exist, puts the fallback language first
        // and sorts the others by code. Problems are reported to the log here.
        // The only place that knows where the list comes from: switching to folder scanning later
        // means rewriting this method only.
        AZStd::vector<LanguageInfo> LoadAvailableLanguages() const;

        // Languages the player can choose. Built once in Activate(), before the first SetLanguage(),
        // because SetLanguage() accepts only codes from this list.
        AZStd::vector<LanguageInfo> m_availableLanguages;

        // Current language code (e.g. "en") and its table always change together.
        AZStd::string m_currentLanguage;
        // Key -> text for m_currentLanguage.
        StringTable m_localizedStrings;
        // Key -> text for the fallback language (currently English).
        // Used when a key is missing in m_localizedStrings.
        StringTable m_fallbackStrings;
        // Keys that have been reported as missing for m_currentLanguage.
        mutable AZStd::unordered_set<AZStd::string> m_reportedMissingKeys;
    };

} // namespace LocalizationGem
