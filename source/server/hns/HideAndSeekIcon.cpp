#include "server/hns/HideAndSeekIcon.h"
#include "server/Client.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/hns/HideAndSeekMode.hpp"
#include "al/util.hpp"

HideAndSeekIcon::HideAndSeekIcon(const char* name, const al::LayoutInitInfo& initInfo) : al::LayoutActor(name) {
    al::initLayoutActor(this, initInfo, "HideAndSeekIcon", 0);

    mInfo = GameModeManager::instance()->getInfo<HideAndSeekInfo>();

    initNerve(&nrvHideAndSeekIconEnd, 0);

    al::hidePane(this, "SeekingIcon");
    al::hidePane(this, "HidingIcon");

    kill();
}

void HideAndSeekIcon::appear() {
    al::startAction(this, "Appear", 0);

    al::setNerve(this, &nrvHideAndSeekIconAppear);

    al::LayoutActor::appear();
}

bool HideAndSeekIcon::tryEnd() {
    if (!al::isNerve(this, &nrvHideAndSeekIconEnd)) {
        al::setNerve(this, &nrvHideAndSeekIconEnd);
        return true;
    }
    return false;
}

bool HideAndSeekIcon::tryStart() {
    if (!al::isNerve(this, &nrvHideAndSeekIconWait) && !al::isNerve(this, &nrvHideAndSeekIconAppear)) {
        appear();
        return true;
    }
    return false;
}

void HideAndSeekIcon::exeAppear() {
    if (al::isActionEnd(this, 0)) {
        al::setNerve(this, &nrvHideAndSeekIconWait);
    }
}

const char* HideAndSeekIcon::getRoleIcon(bool isIt) {
    return isIt ? "\uE002" : "\uE001";
}

GameMode HideAndSeekIcon::getGameMode() {
    return GameMode::HIDEANDSEEK;
}

bool HideAndSeekIcon::isMeIt() {
    return mInfo->mIsPlayerIt;
}

void HideAndSeekIcon::exeWait() {
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

void HideAndSeekIcon::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", 0);
    }

    if (al::isActionEnd(this, 0)) {
        kill();
    }
}

void HideAndSeekIcon::showHiding() {
    al::hidePane(this, "SeekingIcon");
    al::showPane(this, "HidingIcon");
}

void HideAndSeekIcon::showSeeking() {
    al::hidePane(this, "HidingIcon");
    al::showPane(this, "SeekingIcon");
}

const char* getCompassTargetName() {
    // Check if Hide and Seek mode is active
    if (!GameModeManager::instance()->isModeAndActive(GameMode::HIDEANDSEEK)) {
        return nullptr;
    }

    HideAndSeekMode* hnsMode = GameModeManager::instance()->getMode<HideAndSeekMode>();
    if (!hnsMode || !hnsMode->isPlayerSeeking()) {
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
            
            if (puppet->gameMode != GameMode::HIDEANDSEEK && puppet->gameMode != GameMode::LEGACY) {
                continue;
            }
            
            if (puppet->hnsIsHiding()) {
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

const char* HideAndSeekIcon::getCurrentCompassTarget() {
    // If using the getter function approach from PuppetActor.cpp:
    return getCompassTargetName(); // This function handles all the logic
}

namespace {
    NERVE_IMPL(HideAndSeekIcon, Appear)
    NERVE_IMPL(HideAndSeekIcon, Wait)
    NERVE_IMPL(HideAndSeekIcon, End)
}