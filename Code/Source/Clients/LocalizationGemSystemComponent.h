
#pragma once

#include <AzCore/Component/Component.h>
#include <LocalizationGem/LocalizationGemBus.h>
#include <LocalizationContract/LocalizationContractBus.h>

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

        LocalizationGemSystemComponent();
        ~LocalizationGemSystemComponent();

    protected:
        ////////////////////////////////////////////////////////////////////////
        // LocalizationGemRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    };

} // namespace LocalizationGem
