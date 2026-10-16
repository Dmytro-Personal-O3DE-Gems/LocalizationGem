
#pragma once

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Clients/LocalizationGemSystemComponent.h>

namespace LocalizationGem
{
    /// System component for LocalizationGem editor
    class LocalizationGemEditorSystemComponent
        : public LocalizationGemSystemComponent
        , protected AzToolsFramework::EditorEvents::Bus::Handler
    {
        using BaseSystemComponent = LocalizationGemSystemComponent;
    public:
        AZ_COMPONENT_DECL(LocalizationGemEditorSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        LocalizationGemEditorSystemComponent();
        ~LocalizationGemEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
    };
} // namespace LocalizationGem
