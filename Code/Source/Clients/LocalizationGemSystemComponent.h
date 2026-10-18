
#pragma once

#include <AzCore/Component/Component.h>
#include <LocalizationGem/LocalizationGemBus.h>
#include <LocalizationContract/LocalizationContractBus.h>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/string.h>

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
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    private:
        // Key -> text of one language.
        using StringTable = AZStd::unordered_map<AZStd::string, AZStd::string>;

        // Reads "@products@/assets/strings/<languageCode>.csv" and parses it.
        // Rows the parser skipped are reported to the log here, the caller does not see them.
        // Returns nullopt when the file is missing or cannot be read. The caller decides how
        // serious that is: an error for the fallback language, a warning for any other one.
        AZStd::optional<StringTable> LoadStringTable(const AZStd::string& languageCode) const;

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
