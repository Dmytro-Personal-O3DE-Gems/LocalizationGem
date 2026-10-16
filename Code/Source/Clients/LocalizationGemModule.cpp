
#include <LocalizationGem/LocalizationGemTypeIds.h>
#include <LocalizationGemModuleInterface.h>
#include "LocalizationGemSystemComponent.h"

namespace LocalizationGem
{
    class LocalizationGemModule
        : public LocalizationGemModuleInterface
    {
    public:
        AZ_RTTI(LocalizationGemModule, LocalizationGemModuleTypeId, LocalizationGemModuleInterface);
        AZ_CLASS_ALLOCATOR(LocalizationGemModule, AZ::SystemAllocator);
    };
}// namespace LocalizationGem

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), LocalizationGem::LocalizationGemModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_LocalizationGem, LocalizationGem::LocalizationGemModule)
#endif
