
#pragma once

#include <LocalizationGem/LocalizationGemTypeIds.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace LocalizationGem
{
    class LocalizationGemRequests
    {
    public:
        AZ_RTTI(LocalizationGemRequests, LocalizationGemRequestsTypeId);
        virtual ~LocalizationGemRequests() = default;
        // Put your public methods here
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
