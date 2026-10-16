
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
        AZ::TickBus::Handler::BusConnect();
    }

    void LocalizationGemSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        LocalizationGemRequestBus::Handler::BusDisconnect();
    }

    void LocalizationGemSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
    }

} // namespace LocalizationGem
