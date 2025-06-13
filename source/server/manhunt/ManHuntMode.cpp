#include "server/manhunt/ManHuntMode.hpp"

#include "al/util.hpp"
#include "al/util/DemoUtil.h"

#include "game/GameData/GameDataFunction.h"
#include "game/GameData/GameDataHolderAccessor.h"
#include "game/Player/PlayerActorBase.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "game/Player/PlayerFunction.h"

#include "logger.hpp"

#include "rs/util.hpp"
#include "rs/util/PlayerUtil.h"

#include "sead/heap/seadHeapMgr.h"
#include "sead/math/seadVector.h"

#include "server/Client.hpp"
#include "server/DeltaTime.hpp"
#include "server/gamemode/GameModeManager.hpp"
#include "server/gamemode/GameModeFactory.hpp"
#include "server/manhunt/ManHuntPacket.hpp"


ManHuntMode::ManHuntMode(const char* name) : GameModeBase(name) {}

void ManHuntMode::init(const GameModeInitInfo& info) {
    mSceneObjHolder = info.mSceneObjHolder;
    mMode           = info.mMode;
    mCurScene       = (StageScene*)info.mScene;
    mPuppetHolder   = info.mPuppetHolder;

    GameModeInfoBase* curGameInfo = GameModeManager::instance()->getInfo<GameModeInfoBase>();

    sead::ScopedCurrentHeapSetter heapSetter(GameModeManager::instance()->getHeap());

    if (curGameInfo) {
        Logger::log("Gamemode info found: %s %s\n", GameModeFactory::getModeString(curGameInfo->mMode), GameModeFactory::getModeString(info.mMode));
    } else {
        Logger::log("No gamemode info found\n");
    }

    if (curGameInfo && curGameInfo->mMode == mMode) {
        sead::ScopedCurrentHeapSetter heapSetter(GameModeManager::getSceneHeap());
        mInfo = (ManHuntInfo*)curGameInfo;
        mModeTimer = new GameModeTimer(mInfo->mHidingTime);
        Logger::log("Reinitialized timer with time %d:%.2d\n", mInfo->mHidingTime.mMinutes, mInfo->mHidingTime.mSeconds);
    } else {
        if (curGameInfo) {
            delete curGameInfo; // attempt to destory previous info before creating new one
        }

        mInfo = GameModeManager::instance()->createModeInfo<ManHuntInfo>();

        mModeTimer = new GameModeTimer();
    }

    sead::ScopedCurrentHeapSetter heapSetterr(GameModeManager::getSceneHeap());

    mModeLayout = new ManHuntIcon("ManHuntIcon", *info.mLayoutInitInfo);

    mModeLayout->showSeeking();

    mModeTimer->disableTimer();
}

void ManHuntMode::processPacket(Packet* _packet) {
    ManHuntPacket* packet     = (ManHuntPacket*)_packet;
    ManHuntUpdateType      updateType = packet->updateType();

    // if the packet is for our player, edit info for our player
    if (packet->mUserID == Client::getClientId()) {
        if (updateType & ManHuntUpdateType::TIME) {
            mInfo->mHidingTime.mMilliseconds = 0.0;
            mInfo->mHidingTime.mSeconds      = packet->seconds;
            mInfo->mHidingTime.mMinutes      = packet->minutes % 60;
            mInfo->mHidingTime.mHours        = packet->minutes / 60;
            mModeTimer->setTime(mInfo->mHidingTime);
        }

        if (updateType & ManHuntUpdateType::STATE) {
            updateTagState(packet->isIt);
        } else if (updateType & ManHuntUpdateType::TIME) {
            Client::sendGameModeInfPacket();
        }

        return;
    }

    PuppetInfo* other = Client::findPuppetInfo(packet->mUserID, false);
    if (!other) {
        return;
    }

    if (updateType & ManHuntUpdateType::STATE) {
        other->isIt = packet->isIt;
    }

    if (updateType & ManHuntUpdateType::TIME) {
        other->seconds = packet->seconds;
        other->minutes = packet->minutes;
    }
}

Packet* ManHuntMode::createPacket() {
    if (!isModeActive()) {
        DisabledGameModeInf* packet = new DisabledGameModeInf(Client::getClientId());
        return packet;
    }

    ManHuntPacket* packet = new ManHuntPacket();
    packet->mUserID    = Client::getClientId();
    packet->isIt       = isPlayerHunting();
    packet->seconds    = mInfo->mHidingTime.mSeconds;
    packet->minutes    = mInfo->mHidingTime.mMinutes + mInfo->mHidingTime.mHours * 60;
    packet->setUpdateType(static_cast<ManHuntUpdateType>(ManHuntUpdateType::STATE | ManHuntUpdateType::TIME));
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

    // Clean up spectator camera if active
    if (mTicket && mTicket->mIsActive) {
        al::endCamera(mCurScene, mTicket, 0, false);
    }

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

    if (isPlayerHunting()) {
        mModeTimer->disableTimer();
        mModeLayout->showSeeking();
    } else {
        mModeTimer->enableTimer();
        mModeLayout->showHiding();
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

                    if (other->gameMode != mMode && other->gameMode != GameMode::LEGACY) {
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


bool ManHuntMode::showNameTag(PuppetInfo* other) {
    return (
        (other->gameMode != mMode && other->gameMode != GameMode::LEGACY)
        || (isPlayerHunting() && other->manhuntIsHunting())
        || (isPlayerRunning() && other->manhuntIsRunning())
    );
}

void ManHuntMode::debugMenuControls(sead::TextWriter* gTextWriter) {
    gTextWriter->printf("- L + ← | Enable/disable ManHunt \n");
    gTextWriter->printf("-   ↑   | Switch between runner and hunter\n");
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

    Client::sendGameModeInfPacket();
}

void ManHuntMode::onBorderPullBackFirstStep(al::LiveActor* actor) {
    if (isUseGravity()) {
        killMainPlayer(actor);
    }
}

bool ManHuntMode::isPlayerNearOdysseyBarrier(al::LiveActor* player) {
    if (!player) return false;

    PlayerActorBase* playerBase = rs::getPlayerActor(mCurScene);
    if (!playerBase) return false;

    ShineTowerRocket* odyssey = rs::tryGetShineTowerRocketFromDemoDirector((al::LiveActor*)playerBase);
    if (!odyssey) return false;

    sead::Vector3f odysseyPos = al::getTrans((al::LiveActor*)odyssey);
    
    // Check if the current player is a hider and within barrier range
    if (player == (al::LiveActor*)playerBase) {
        // This is the main player - check if they're hiding
        if (!isPlayerHunting()) {
            sead::Vector3f playerPos = al::getTrans(player);
            f32 distance = vecDistance(playerPos, odysseyPos);
            return distance < 2045.0f;
        }
    } else {
        // This is a puppet - need to find their PuppetInfo to check if they're hiding
        for (size_t i = 0; i < (size_t)mPuppetHolder->getSize(); i++) {
            PuppetInfo* puppetInfo = Client::getPuppetInfo(i);
            if (!puppetInfo || !puppetInfo->isConnected || !puppetInfo->isInSameStage) {
                continue;
            }
            
            // Check if this puppet corresponds to the player we're checking
            // You might need to adjust this comparison based on how you identify puppets
            if (puppetInfo->manhuntIsRunning()) {
                f32 distance = vecDistance(puppetInfo->playerPos, odysseyPos);
                if (distance < 2045.0f) {
                    return true;
                }
            }
        }
    }
    
    return false;
}