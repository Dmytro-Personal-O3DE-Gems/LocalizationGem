
#include "LocalizationGemSystemComponent.h"

#include <LocalizationGem/LocalizationGemTypeIds.h>
#include <AzCore/Utils/Utils.h>
#include <StringTableCsvParser.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace LocalizationGem
{
    namespace
    {
        // The fallback language is hardcoded to English for now, until there is a way to choose it.
        static constexpr const char* FallbackLanguage = "en";
    }

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
        // 1. The current language.
        if (auto it = m_localizedStrings.find(key); it != m_localizedStrings.end())
        {
            return it->second;
        }

        // Steps 2 and 3 warn about a key only the first time: the prompt asks for its labels every tick,
        // so without this the same warning would be printed on every call.
        // insert(...).second is true only when the key was not in the set yet.
        // One set is enough for both warnings: for one language a key ends up either in step 2 or in step 3.

        // 2. The fallback language: the key exists, but is not translated into the current language.
        if (auto it = m_fallbackStrings.find(key); it != m_fallbackStrings.end())
        {
            if (m_reportedMissingKeys.insert(key).second)
            {
                AZ_Warning("LocalizationGem", false,
                    "Key '%s' is not translated into '%s', using '%s'.",
                    key.c_str(), m_currentLanguage.c_str(), FallbackLanguage);
            }
            return it->second;
        }

        // 3. Nowhere: usually a typo in the key, or a key that was never added to the fallback table.
        if (m_reportedMissingKeys.insert(key).second)
        {
            AZ_Warning("LocalizationGem", false,
                "No text for key '%s' in '%s' or '%s'. "
                "The consumer will fall back to its own default text.",
                key.c_str(), m_currentLanguage.c_str(), FallbackLanguage);
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
        //    Two tables are loaded from the cache (assets/strings/<language>.csv):
        //    - the fallback language (English): used when a key is missing in the current language;
        //    - the current language: what the player normally sees.
        //    TEMPORARY: the current language is hardcoded to Spanish until there is a way to choose it.
        //    es.csv has no "door.force" on purpose, to see the fallback.
        if (auto table = LoadStringTable(FallbackLanguage); table.has_value())
        {
            m_fallbackStrings = AZStd::move(*table);
        }
        else
        {
            // LoadStringTable has already logged why the file could not be read.
            AZ_Error("LocalizationGem", false,
                "Cannot load the fallback string table '%s'. Players will see default texts instead of translations.",
                FallbackLanguage);
        }

        m_currentLanguage = "es";
        if (auto table = LoadStringTable(m_currentLanguage); table.has_value())
        {
            m_localizedStrings = AZStd::move(*table);
        }
        else
        {
            AZ_Warning("LocalizationGem", false,
                "Cannot load the string table '%s'. Texts will be shown in '%s' instead.",
                m_currentLanguage.c_str(), FallbackLanguage);
        }

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
        m_fallbackStrings.clear();
        m_reportedMissingKeys.clear();
    }

    // The return type stands before "LocalizationGemSystemComponent::", where names declared
    // inside the class are not visible yet, so StringTable needs the class prefix here.
    AZStd::optional<LocalizationGemSystemComponent::StringTable> LocalizationGemSystemComponent::LoadStringTable(
        const AZStd::string& languageCode) const
    {
        AZStd::string filePath = AZStd::string::format("@products@/assets/strings/%s.csv", languageCode.c_str());
        auto result = AZ::Utils::ReadFile(filePath);

        if (!result.IsSuccess())
        {
            // Only this method knows why the file could not be read (missing, empty, ...).
            // The caller adds its own message about how serious that is.
            AZ_Warning("LocalizationGem", false, "Cannot load the string table for language '%s': %s",
                languageCode.c_str(), result.GetError().c_str());
            return AZStd::nullopt;
        }

        StringTableParseResult tableResult = ParseStringTableCsv(result.GetValue());

        for (const auto& problem : tableResult.m_problems)
        {
            AZ_Warning("LocalizationGem", false, "Problem in %s line %d: %s",
                filePath.c_str(), problem.m_line, problem.m_message.c_str());
        }

        // m_strings is a member of a local variable, so return does not move it by itself:
        // without AZStd::move every string of the table would be copied.
        return AZStd::move(tableResult.m_strings);
    }

} // namespace LocalizationGem
