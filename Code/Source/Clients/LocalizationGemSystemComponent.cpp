
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

    AZStd::optional<AZStd::string> LocalizationGemSystemComponent::GetLocalizedString(const AZStd::string& key) const
    {
        auto it = m_localizedStrings.find(key);
        if (it != m_localizedStrings.end())
        {
            return it->second;
        }

        // The prompt asks for its labels every tick, so a missing key would be
        // reported on every call. Warn only the first time for each key:
        // insert(...).second is true only when the key was not in the set yet.
        if (m_reportedMissingKeys.insert(key).second)
        {
            AZ_Warning("LocalizationGem", false,
                "No text for key '%s' in language '%s'. "
                "The consumer will fall back to its own default text.",
                key.c_str(), m_currentLanguage.c_str());
        }

        return AZStd::nullopt;
    }

    AZStd::string LocalizationGemSystemComponent::GetCurrentLanguage() const
    {
        return m_currentLanguage;
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
        // 1. Prepare the data first, so nobody can reach a half-ready component.
        //    First version: a hardcoded table. CSV loading will replace this block.
        //    TEMPORARY test data: Spanish, so a translated label differs from its English default text,
        //    and "door.force" is left out on purpose to see the fallback to the default text.
        m_currentLanguage = "es";
        m_localizedStrings.emplace("door.open", "Abrir");
        m_localizedStrings.emplace("door.close", "Cerrar");
        m_localizedStrings.emplace("door.lock", "Bloquear");

        // 2. Go live: own API, then both access paths required by LocalizationContract.
        LocalizationGemRequestBus::Handler::BusConnect();

        LocalizationContract::LocalizationContractRequestBus::Handler::BusConnect();
        // Register() does not log anything on failure, it only returns an Outcome.
        const auto registerOutcome = LocalizationContract::LocalizationContractInterface::Register(this);
        AZ_Error("LocalizationGem", registerOutcome.IsSuccess(),
            "Failed to register as the LocalizationContract implementation: %s",
            registerOutcome.GetError().c_str());
    }

    void LocalizationGemSystemComponent::Deactivate()
    {
        // Mirror order of Activate(): leave first, then destroy the data.
        LocalizationContract::LocalizationContractRequestBus::Handler::BusDisconnect();
        // Unregister only if it is really us: if Register() failed in Activate(), Unregister() would fail too.
        if (LocalizationContract::LocalizationContractInterface::Get() == this)
        {
            LocalizationContract::LocalizationContractInterface::Unregister(this);
        }

        LocalizationGemRequestBus::Handler::BusDisconnect();

        m_localizedStrings.clear();
        m_currentLanguage.clear();
    }

} // namespace LocalizationGem
