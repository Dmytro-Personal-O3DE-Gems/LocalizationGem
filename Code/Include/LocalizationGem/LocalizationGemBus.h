
#pragma once

#include <LocalizationGem/LocalizationGemTypeIds.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace LocalizationGem
{
    //! One entry of the language menu.
    struct LanguageInfo
    {
        //! Language code, e.g. "uk". Pass it to LocalizationGemRequests::SetLanguage.
        AZStd::string m_code;
        //! Name of the language in that language itself, e.g. "Українська". Show it in the menu as is.
        AZStd::string m_displayName;
    };

    class LocalizationGemRequests
    {
    public:
        AZ_RTTI(LocalizationGemRequests, LocalizationGemRequestsTypeId);
        virtual ~LocalizationGemRequests() = default;
        //! Switches the current language, e.g. SetLanguage("uk").
        //! Loads "assets/strings/<languageCode>.csv" and, on success, replaces the current texts and sends
        //! LocalizationContractNotifications::OnLocalizedStringsChanged, so listeners can request their texts again.
        //! Does nothing when languageCode is already the current language.
        //! Codes that are not in GetAvailableLanguages() are rejected with a warning.
        //! If the file cannot be loaded, the previous language stays active and the reason is written to the log.
        virtual void SetLanguage(const AZStd::string& languageCode) = 0;

        //! Languages the player can choose, read once at startup from "assets/strings/languages.csv".
        //! Only languages whose string table file exists are listed.
        //! The fallback language (English) comes first, the others are sorted by code.
        //! Returns a copy: the caller may keep it as long as it needs.
        virtual AZStd::vector<LanguageInfo> GetAvailableLanguages() const = 0;
    };

    class LocalizationGemBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using LocalizationGemRequestBus = AZ::EBus<LocalizationGemRequests, LocalizationGemBusTraits>;
    using LocalizationGemInterface = AZ::Interface<LocalizationGemRequests>;

} // namespace LocalizationGem
