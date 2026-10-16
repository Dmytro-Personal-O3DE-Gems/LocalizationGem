
#pragma once

namespace LocalizationGem
{
    // System Component TypeIds
    inline constexpr const char* LocalizationGemSystemComponentTypeId = "{81F39AE6-B06C-479A-89B3-66D75E53A306}";
    inline constexpr const char* LocalizationGemEditorSystemComponentTypeId = "{C8C003E4-E1B5-4B97-A8A0-2745ABC4D993}";

    // Module derived classes TypeIds
    inline constexpr const char* LocalizationGemModuleInterfaceTypeId = "{F12AA533-CB41-4326-903F-1FC11CFA6801}";
    inline constexpr const char* LocalizationGemModuleTypeId = "{3A6DB070-F1A0-4EFE-8256-674EF17BA071}";
    // The Editor Module by default is mutually exclusive with the Client Module
    // so they use the Same TypeId
    inline constexpr const char* LocalizationGemEditorModuleTypeId = LocalizationGemModuleTypeId;

    // Interface TypeIds
    inline constexpr const char* LocalizationGemRequestsTypeId = "{A9D720D4-4A95-46B0-9BFE-B2968992DD39}";
} // namespace LocalizationGem
