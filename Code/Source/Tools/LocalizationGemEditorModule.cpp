
#include <LocalizationGem/LocalizationGemTypeIds.h>
#include <LocalizationGemModuleInterface.h>
#include "LocalizationGemEditorSystemComponent.h"

namespace LocalizationGem
{
    class LocalizationGemEditorModule
        : public LocalizationGemModuleInterface
    {
    public:
        AZ_RTTI(LocalizationGemEditorModule, LocalizationGemEditorModuleTypeId, LocalizationGemModuleInterface);
        AZ_CLASS_ALLOCATOR(LocalizationGemEditorModule, AZ::SystemAllocator);

        LocalizationGemEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                LocalizationGemEditorSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<LocalizationGemEditorSystemComponent>(),
            };
        }
    };
}// namespace LocalizationGem

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), LocalizationGem::LocalizationGemEditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_LocalizationGem_Editor, LocalizationGem::LocalizationGemEditorModule)
#endif
