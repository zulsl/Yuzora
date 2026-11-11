#include "ApplicationBootstrap.h"

#include "DatabaseManager.h"
#include "ConnectionProviderRegistry.h"
#include "ApiConfigService.h"
#include "CharacterService.h"
#include "CompletionPresetService.h"
#include "SettingsService.h"

bool ApplicationBootstrap::initialize()
{
    if (!DatabaseManager::instance().initialize()) {
        return false;
    }

    ConnectionProviderRegistry::setProvider(&DatabaseManager::instance());

    CharacterService::instance().ensureSeedData();
    CompletionPresetService::instance().ensureSeedData();
    ApiConfigService::instance().ensureSeedData();
    SettingsService::instance().reloadFromDatabase();

    return true;
}
