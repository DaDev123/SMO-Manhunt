#include "server/manhunt/ManHuntConfigMenu.hpp"
#include <cmath>
#include "logger.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/manhunt/ManHuntMode.hpp"
#include "server/Client.hpp"

ManHuntConfigMenu::ManHuntConfigMenu() : GameModeConfigMenu() {}

void ManHuntConfigMenu::initMenu(const al::LayoutInitInfo &initInfo) {
    
}

const sead::WFixedSafeString<0x200> *ManHuntConfigMenu::getStringData() {
    sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>* gamemodeConfigOptions =
        new sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>();

    gamemodeConfigOptions->mBuffer[0].copy(u"Toggle ManHunt Gravity On");
    gamemodeConfigOptions->mBuffer[1].copy(u"Toggle ManHunt Gravity Off");

    return gamemodeConfigOptions->mBuffer;
}

bool ManHuntConfigMenu::updateMenu(int selectIndex) {

    ManHuntInfo *manhuntMode = GameModeManager::instance()->getInfo<ManHuntInfo>();

    Logger::log("Setting Gravity Mode.\n");

    if (!manhuntMode) {
        Logger::log("Unable to Load Mode info!\n");
        return true;   
    }
    
    switch (selectIndex) {
        case 0: {
            if (GameModeManager::instance()->isMode(GameMode::MANHUNT)) {
                manhuntMode->mIsUseGravity = true;
            }
            return true;
        }
        case 1: {
            if (GameModeManager::instance()->isMode(GameMode::MANHUNT)) {
                manhuntMode->mIsUseGravity = false;
            }
            return true;
        }
        default:
            Logger::log("Failed to interpret Index!\n");
            return false;
    }
    
}