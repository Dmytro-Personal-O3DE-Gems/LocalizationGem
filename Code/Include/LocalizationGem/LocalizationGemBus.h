
#pragma once

#include <LocalizationGem/LocalizationGemTypeIds.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/string/string.h>

namespace LocalizationGem
{
    class LocalizationGemRequests
    {
    public:
        AZ_RTTI(LocalizationGemRequests, LocalizationGemRequestsTypeId);
        virtual ~LocalizationGemRequests() = default;
        //! Switches the current language, e.g. SetLanguage("uk").
        //! Loads "assets/strings/<languageCode>.csv" and, on success, replaces the current texts and sends
        //! LocalizationContractNotifications::OnLocalizedStringsChanged, so listeners can request their texts again.
        //! Does nothing when languageCode is already the current language.
        //! If the file cannot be loaded, the previous language stays active and the reason is written to the log.
        virtual void SetLanguage(const AZStd::string& languageCode) = 0;
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
