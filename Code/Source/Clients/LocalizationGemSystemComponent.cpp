
#include "LocalizationGemSystemComponent.h"

#include <LocalizationGem/LocalizationGemTypeIds.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace LocalizationGem
{
    AZ_COMPONENT_IMPL(LocalizationGemSystemComponent, "LocalizationGemSystemComponent",
        LocalizationGemSystemComponentTypeId);

    void LocalizationGemSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<LocalizationGemSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void LocalizationGemSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("LocalizationGemService"));
    }

    void LocalizationGemSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("LocalizationGemService"));
    }

    void LocalizationGemSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void LocalizationGemSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    LocalizationGemSystemComponent::LocalizationGemSystemComponent()
    {
        if (LocalizationGemInterface::Get() == nullptr)
        {
            LocalizationGemInterface::Register(this);
        }
    }

    LocalizationGemSystemComponent::~LocalizationGemSystemComponent()
    {
        if (LocalizationGemInterface::Get() == this)
        {
            LocalizationGemInterface::Unregister(this);
        }
    }

    void LocalizationGemSystemComponent::Init()
    {
    }

    void LocalizationGemSystemComponent::Activate()
    {
        LocalizationGemRequestBus::Handler::BusConnect();

        LocalizationContract::LocalizationContractRequestBus::Handler::BusConnect();
        LocalizationContract::LocalizationContractInterface::Register(this);
        AZ_Error("LocalizationGem", registerOutcome.IsSuccess(),
            "Failed to register as the LocalizationContract implementation: %s",
            registerOutcome.GetError().c_str());
    }

    void LocalizationGemSystemComponent::Deactivate()
    {
        LocalizationGemRequestBus::Handler::BusDisconnect();

        LocalizationContract::LocalizationContractRequestBus::Handler::BusDisconnect();
        LocalizationContract::LocalizationContractInterface::Unregister(this);
    }

} // namespace LocalizationGem
