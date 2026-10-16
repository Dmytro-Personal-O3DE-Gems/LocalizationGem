
#include "LocalizationGemModuleInterface.h"
#include <AzCore/Memory/Memory.h>

#include <LocalizationGem/LocalizationGemTypeIds.h>

#include <Clients/LocalizationGemSystemComponent.h>

namespace LocalizationGem
{
    AZ_TYPE_INFO_WITH_NAME_IMPL(LocalizationGemModuleInterface,
        "LocalizationGemModuleInterface", LocalizationGemModuleInterfaceTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL(LocalizationGemModuleInterface, AZ::Module);
    AZ_CLASS_ALLOCATOR_IMPL(LocalizationGemModuleInterface, AZ::SystemAllocator);

    LocalizationGemModuleInterface::LocalizationGemModuleInterface()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        // Add ALL components descriptors associated with this gem to m_descriptors.
        // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
        // This happens through the [MyComponent]::Reflect() function.
        m_descriptors.insert(m_descriptors.end(), {
            LocalizationGemSystemComponent::CreateDescriptor(),
            });
    }

    AZ::ComponentTypeList LocalizationGemModuleInterface::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<LocalizationGemSystemComponent>(),
        };
    }
} // namespace LocalizationGem
