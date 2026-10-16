
#include <AzCore/Serialization/SerializeContext.h>
#include "LocalizationGemEditorSystemComponent.h"

#include <LocalizationGem/LocalizationGemTypeIds.h>

namespace LocalizationGem
{
    AZ_COMPONENT_IMPL(LocalizationGemEditorSystemComponent, "LocalizationGemEditorSystemComponent",
        LocalizationGemEditorSystemComponentTypeId, BaseSystemComponent);

    void LocalizationGemEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<LocalizationGemEditorSystemComponent, LocalizationGemSystemComponent>()
                ->Version(0);
        }
    }

    LocalizationGemEditorSystemComponent::LocalizationGemEditorSystemComponent() = default;

    LocalizationGemEditorSystemComponent::~LocalizationGemEditorSystemComponent() = default;

    void LocalizationGemEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("LocalizationGemEditorService"));
    }

    void LocalizationGemEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("LocalizationGemEditorService"));
    }

    void LocalizationGemEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void LocalizationGemEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void LocalizationGemEditorSystemComponent::Activate()
    {
        LocalizationGemSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void LocalizationGemEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        LocalizationGemSystemComponent::Deactivate();
    }

} // namespace LocalizationGem
