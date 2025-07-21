#include "server/manhunt/ManHuntMode.hpp"
#include <cmath>
#include "al/async/FunctorV0M.hpp"
#include "al/util.hpp"
#include "al/util/DemoUtil.h"
#include "al/util/ControllerUtil.h"
#include "al/util/LiveActorUtil.h"
#include "game/GameData/GameDataHolderAccessor.h"
#include "game/Layouts/CoinCounter.h"
#include "game/Layouts/MapMini.h"
#include "game/Player/PlayerActorBase.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "heap/seadHeapMgr.h"
#include "layouts/ManHuntIcon.h"
#include "logger.hpp"
#include "math/seadVector.h"
#include "packets/Packet.h"
#include "rs/util.hpp"
#include "rs/util/PlayerUtil.h"
#include "server/gamemode/GameModeBase.hpp"
#include "server/Client.hpp"
#include "server/gamemode/GameModeTimer.hpp"
#include <heap/seadHeap.h>
#include <math.h>
#include "server/gamemode/GameModeManager.hpp"
#include "server/gamemode/GameModeFactory.hpp"

#include "basis/seadNew.h"
#include "server/manhunt/ManHuntConfigMenu.hpp"

ManHuntMode::ManHuntMode(const char* name) : GameModeBase(name) {}

void ManHuntMode::init(const GameModeInitInfo& info) {
    mSceneObjHolder = info.mSceneObjHolder;
    mMode = info.mMode;
    mCurScene = (StageScene*)info.mScene;
    mPuppetHolder = info.mPuppetHolder;

    GameModeInfoBase* curGameInfo = GameModeManager::instance()->getInfo<ManHuntInfo>();

    if (curGameInfo) Logger::log("Gamemode info found: %s %s\n", GameModeFactory::getModeString(curGameInfo->mMode), GameModeFactory::getModeString(info.mMode));
    else Logger::log("No gamemode info found\n");
    if (curGameInfo && curGameInfo->mMode == mMode) {
        mInfo = (ManHuntInfo*)curGameInfo;
        mModeTimer = new GameModeTimer(mInfo->mHidingTime);
        Logger::log("Reinitialized timer with time %d:%.2d\n", mInfo->mHidingTime.mMinutes, mInfo->mHidingTime.mSeconds);
    } else {
        if (curGameInfo) delete curGameInfo;  // attempt to destory previous info before creating new one
        
        mInfo = GameModeManager::instance()->createModeInfo<ManHuntInfo>();
        
        mModeTimer = new GameModeTimer();
    }

    mModeLayout = new ManHuntIcon("ManHuntIcon", *info.mLayoutInitInfo);

    mModeLayout->showSeeking();

    mModeTimer->disableTimer();

}

void ManHuntMode::processPacket(Packet *packet) {
    ManHuntPacket* tagPacket = (ManHuntPacket*)packet;

    // if the packet is for our player, edit info for our player
    if (tagPacket->mUserID == Client::getClientId() && GameModeManager::instance()->isMode(GameMode::MANHUNT)) {

        ManHuntMode* mode = GameModeManager::instance()->getMode<ManHuntMode>();
        ManHuntInfo* curInfo = GameModeManager::instance()->getInfo<ManHuntInfo>();

        if (tagPacket->updateType & TagUpdateType::STATE) {
            mode->setPlayerTagState(tagPacket->isIt);
        }

        if (tagPacket->updateType & TagUpdateType::TIME) {
            curInfo->mHidingTime.mSeconds = tagPacket->seconds;
            curInfo->mHidingTime.mMinutes = tagPacket->minutes;
        }

        return;

    }

    PuppetInfo* curInfo = Client::findPuppetInfo(tagPacket->mUserID, false);

    if (!curInfo) {
        return;
    }

    curInfo->isIt = tagPacket->isIt;
    curInfo->seconds = tagPacket->seconds;
    curInfo->minutes = tagPacket->minutes;
}

Packet *ManHuntMode::createPacket() {

    ManHuntPacket *packet = new ManHuntPacket();

    packet->mUserID = Client::getClientId();

    packet->isIt = isPlayerHunting();

    packet->minutes = mInfo->mHidingTime.mMinutes;
    packet->seconds = mInfo->mHidingTime.mSeconds;
    packet->updateType = static_cast<TagUpdateType>(TagUpdateType::STATE | TagUpdateType::TIME);

    return packet;
}

void ManHuntMode::begin() {
    unpause();

    mIsFirstFrame = true;
    mInvulnTime   = 0.0f;

    // Set initial Kids Mode state based on player role
    GameDataHolderAccessor accessor(this);
    if (accessor.mData && accessor.mData->mGameDataFile) {
        // If player starts as seeker, disable Kids Mode; if hider, enable it
        bool initialKidsMode = !mInfo->mIsPlayerIt; // true for hiders, false for seekers
        accessor.mData->mGameDataFile->setKidsMode(initialKidsMode);
    }

    GameModeBase::begin();
}



void ManHuntMode::end() {

    pause();

    GameModeBase::end();
}

void ManHuntMode::pause() {
    GameModeBase::pause();

    mModeLayout->tryEnd();
    mModeTimer->disableTimer();
}

void ManHuntMode::unpause() {
    GameModeBase::unpause();

    mModeLayout->appear();
    
    if (!mInfo->mIsPlayerIt) {
        mModeTimer->enableTimer();
        mModeLayout->showHiding();
    } else {
        mModeTimer->disableTimer();
        mModeLayout->showSeeking();
    }
}

void ManHuntMode::update() {
    PlayerActorBase* playerBase = rs::getPlayerActor(mCurScene);
    bool isYukimaru = !playerBase->getPlayerInfo();

    if (mIsFirstFrame) {
        if (mInfo->mIsUseGravityCam && mTicket) {
            al::startCamera(mCurScene, mTicket, -1);
        }
        mIsFirstFrame = false;
    }

    if (rs::isActiveDemoPlayerPuppetable(playerBase)) {
        mInvulnTime = 0.0f;
    }

    bool isSpectatorCameraActive = mTicket && mTicket->mIsActive;

    ShineTowerRocket* odyssey = rs::tryGetShineTowerRocketFromDemoDirector((al::LiveActor*)playerBase);
    if (odyssey) {
        if (GameModeManager::instance()->isModeAndActive(GameMode::MANHUNT) && !isYukimaru) {
            al::tryEmitEffect((al::LiveActor*)odyssey, "Special1WorldHomeGKBarrier", al::getTransPtr((al::LiveActor*)odyssey));
            al::setEffectParticleScale((al::LiveActor*)odyssey, "Special1WorldHomeGKBarrier", 1.3f);

            bool anyHiderClose = false;
            sead::Vector3f odysseyPos = al::getTrans((al::LiveActor*)odyssey);

            if (!isPlayerHunting()) {
                f32 distance = al::calcDistanceH((al::LiveActor*)playerBase, (al::LiveActor*)odyssey);
                if (distance < 2045.0f) {
                    anyHiderClose = true;
                }
            }

            if (!anyHiderClose) {
                for (size_t i = 0; i < (size_t)mPuppetHolder->getSize(); i++) {
                    PuppetInfo* other = Client::getPuppetInfo(i);
                    if (!other || !other->isConnected || !other->isInSameStage) {
                        continue;
                    }

                    if (other->manhuntIsRunning()) {
                        f32 distance = vecDistance(other->playerPos, odysseyPos);
                        if (distance < 2045.0f) {
                            anyHiderClose = true;
                            break;
                        }
                    }
                }
            }

            if (anyHiderClose) {
                al::setEffectParticleColor((al::LiveActor*)odyssey, "Special1WorldHomeGKBarrier", sead::Color4f(0.0f, 1.0f, 0.0f, 1.0f));
            } else {
                al::setEffectParticleColor((al::LiveActor*)odyssey, "Special1WorldHomeGKBarrier", sead::Color4f(1.0f, 0.0f, 0.0f, 1.0f));
            }
        } else {
            al::tryDeleteEffect((al::LiveActor*)odyssey, "Special1WorldHomeGKBarrier");
        }
    }

    if (isPlayerHunting()) {
        if (!isSpectatorCameraActive) {
            mModeTimer->timerControl();
        }
    } else {
        if (mInvulnTime < 5) {
            mInvulnTime += Time::deltaTime;
        }

        if (!isSpectatorCameraActive) {
            mModeTimer->updateTimer();
        }
    }

    if (mInfo->mIsUseGravity && !isYukimaru) {
        sead::Vector3f gravity;
        if (rs::calcOnGroundNormalOrGravityDir(&gravity, playerBase, playerBase->getPlayerCollision())) {
            gravity = -gravity;
            al::normalize(&gravity);
            al::setGravity(playerBase, gravity);
            al::setGravity(((PlayerActorHakoniwa*)playerBase)->mHackCap, gravity);
        }

        if (al::isPadHoldL(-1)) {
            if (al::isPadTriggerRight(-1)) {
                if (al::isActiveCamera(mTicket)) {
                    al::endCamera(mCurScene, mTicket, -1, false);
                    mInfo->mIsUseGravityCam = false;
                } else {
                    al::startCamera(mCurScene, mTicket, -1);
                    mInfo->mIsUseGravityCam = true;
                }
            }
        } else if (al::isPadTriggerZL(-1) && al::isPadTriggerLeft(-1)) {
            killMainPlayer(((PlayerActorHakoniwa*)playerBase));
        }
    }

    mInfo->mHidingTime = mModeTimer->getTime();

    // Switch roles - only if R is NOT being held
    if (al::isPadTriggerUp(-1) && !al::isPadHoldZL(-1) && !al::isPadHoldR(-1)) {
        updateTagState(isPlayerRunning());
    }

    //bool toggleComboPressed = al::isPadHoldR(-1) && al::isPadTriggerUp(-1);
// if (toggleComboPressed) {
//     if (!mTicket->mIsActive) {
//         al::startCamera(mCurScene, mTicket, -1);
//         al::requestStopCameraVerticalAbsorb(mCurScene);
//     } else {
//         al::endCamera(mCurScene, mTicket, 0, false);
//         al::requestStopCameraVerticalAbsorb(mCurScene);
//     }
// }
// 
// if (mTicket->mIsActive) {
//     updateSpectateCam(playerBase);
// }
}

bool ManHuntMode::isPlayerInSafeZone(al::LiveActor* player) {
    if (!player) return false;
    
    // Get the Odyssey (barrier source)
    PlayerActorBase* playerBase = rs::getPlayerActor(mCurScene);
    if (!playerBase) return false;
    
    ShineTowerRocket* odyssey = rs::tryGetShineTowerRocketFromDemoDirector((al::LiveActor*)playerBase);
    if (!odyssey) return false;
    
    // Check distance to Odyssey barrier (same distance used for barrier effect)
    f32 distance = al::calcDistanceH(player, (al::LiveActor*)odyssey);
    return distance < 2045.0f; // Same distance as in your update() method
}

void ManHuntMode::updateTagState(bool isHunting) {
    mInfo->mIsPlayerIt = isHunting;

    if (isHunting) {
        mModeTimer->disableTimer();
        mModeLayout->showSeeking();
        
        // Seeker: disable Kids Mode
        GameDataHolderAccessor accessor(this);
        if (accessor.mData && accessor.mData->mGameDataFile) {
            accessor.mData->mGameDataFile->setKidsMode(false);
        }
    } else {
        mModeTimer->enableTimer();
        mModeLayout->showHiding();
        mInvulnTime = 0;
        
        // Hider: enable Kids Mode
        GameDataHolderAccessor accessor(this);
        if (accessor.mData && accessor.mData->mGameDataFile) {
            accessor.mData->mGameDataFile->setKidsMode(true);
        }
    }

    Client::sendGamemodePacket();
}