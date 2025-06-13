#include "server/manhunt/ManHuntIcon.h"
#include "server/Client.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/manhunt/ManHuntMode.hpp"
#include "al/util.hpp"

ManHuntIcon::ManHuntIcon(const char* name, const al::LayoutInitInfo& initInfo) : al::LayoutActor(name) {
    al::initLayoutActor(this, initInfo, "ManHuntIcon", 0);

    mInfo = GameModeManager::instance()->getInfo<ManHuntInfo>();

    initNerve(&nrvManHuntIconEnd, 0);

    al::hidePane(this, "SeekingIcon");
    al::hidePane(this, "HidingIcon");

    kill();
}

void ManHuntIcon::appear() {
    al::startAction(this, "Appear", 0);

    al::setNerve(this, &nrvManHuntIconAppear);

    al::LayoutActor::appear();
}

bool ManHuntIcon::tryEnd() {
    if (!al::isNerve(this, &nrvManHuntIconEnd)) {
        al::setNerve(this, &nrvManHuntIconEnd);
        return true;
    }
    return false;
}

bool ManHuntIcon::tryStart() {
    if (!al::isNerve(this, &nrvManHuntIconWait) && !al::isNerve(this, &nrvManHuntIconAppear)) {
        appear();
        return true;
    }
    return false;
}

void ManHuntIcon::exeAppear() {
    if (al::isActionEnd(this, 0)) {
        al::setNerve(this, &nrvManHuntIconWait);
    }
}

const char* ManHuntIcon::getRoleIcon(bool isIt) {
    return isIt ? "\uE002" : "\uE001";
}

GameMode ManHuntIcon::getGameMode() {
    return GameMode::MANHUNT;
}

bool ManHuntIcon::isMeIt() {
    return mInfo->mIsPlayerIt;
}

void ManHuntIcon::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", 0);
    }

    al::setPaneStringFormat(this, "TxtCounter", mInfo->mHidingTime.to_string().cstr());

    auto playerList = getPlayerList();
    if (!playerList.isEmpty()) {
        al::setPaneStringFormat(this, "TxtPlayerList", playerList.cstr());
    }

    // Show current compass target
    if (mInfo->mIsPlayerIt) { // Only show for seekers
        const char* targetName = getCurrentCompassTarget();
        if (targetName) {
            al::setPaneStringFormat(this, "TxtTarget", targetName);
        } else {
            al::setPaneStringFormat(this, "TxtTarget", "No Target");
        }
    } else {
        // Hide compass target text for hiders
        al::setPaneStringFormat(this, "TxtTarget", "");
    }
}

void ManHuntIcon::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", 0);
    }

    if (al::isActionEnd(this, 0)) {
        kill();
    }
}

void ManHuntIcon::showHiding() {
    al::hidePane(this, "SeekingIcon");
    al::showPane(this, "HidingIcon");
}

void ManHuntIcon::showSeeking() {
    al::hidePane(this, "HidingIcon");
    al::showPane(this, "SeekingIcon");
}

const char* getCompassTargetName() {
    // Check if Hide and Seek mode is active
    if (!GameModeManager::instance()->isModeAndActive(GameMode::MANHUNT)) {
        return nullptr;
    }

    ManHuntMode* manhuntMode = GameModeManager::instance()->getMode<ManHuntMode>();
    if (!manhuntMode || !manhuntMode->isPlayerHunting()) {
        return nullptr;
    }

    // Get list of valid hiders (including those in different stages)
    PuppetInfo* validHiders[32];
    int validHiderCount = 0;
    
    PuppetHolder* puppetHolder = Client::getPuppetHolder();
    if (puppetHolder) {
        for (size_t i = 0; i < (size_t)puppetHolder->getSize() && validHiderCount < 32; i++) {
            PuppetInfo* puppet = Client::getPuppetInfo(i);
            if (!puppet || !puppet->isConnected) {
                continue;
            }
            
            if (puppet->gameMode != GameMode::MANHUNT && puppet->gameMode != GameMode::LEGACY) {
                continue;
            }
            
            if (puppet->manhuntIsRunning()) {
                validHiders[validHiderCount] = puppet;
                validHiderCount++;
            }
        }
    }

    if (validHiderCount == 0) {
        return nullptr;
    }

    int maxTargets = validHiderCount; // Removed +1 for "closest" option
    int currentIndex = compassTargetIndex;
    
    // Clamp index
    if (currentIndex >= maxTargets) {
        currentIndex = 0;
    }

    // Point to specific player (no "closest" option)
    if (currentIndex < validHiderCount && validHiders[currentIndex]) {
        return validHiders[currentIndex]->puppetName;
    }

    return nullptr;
}

const char* ManHuntIcon::getCurrentCompassTarget() {
    // If using the getter function approach from PuppetActor.cpp:
    return getCompassTargetName(); // This function handles all the logic
}

namespace {
    NERVE_IMPL(ManHuntIcon, Appear)
    NERVE_IMPL(ManHuntIcon, Wait)
    NERVE_IMPL(ManHuntIcon, End)
}