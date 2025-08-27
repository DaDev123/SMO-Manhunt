#include "server/manhunt/ManHuntMode.hpp"
#include <cmath>
#include "main.hpp"
#include "al/async/FunctorV0M.hpp"
#include "al/util.hpp"
#include "al/util/ControllerUtil.h"
#include "game/GameData/GameDataHolderAccessor.h"
#include "game/Layouts/CoinCounter.h"
#include "game/Layouts/MapMini.h"
#include "game/Player/PlayerActorBase.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "heap/seadHeapMgr.h"
#include "layouts/ManHuntIcon.h"
#include "logger.hpp"
#include "rs/util.hpp"
#include "server/gamemode/GameModeBase.hpp"
#include "server/Client.hpp"
#include "server/gamemode/GameModeTimer.hpp"
#include <heap/seadHeap.h>
#include "server/gamemode/GameModeManager.hpp"
#include "server/gamemode/GameModeFactory.hpp"
#include "al/util/DemoUtil.h"

#include "basis/seadNew.h"
#include "server/manhunt/ManHuntConfigMenu.hpp"

#include "al/sensor/HitSensor.h"
#include "al/util/SensorUtil.h"
#include "rs/util/SensorUtil.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "actors/PuppetActor.h"
#include "server/Client.hpp"

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

void ManHuntMode::begin() {
    mModeLayout->appear();

    mIsFirstFrame = true;

    GameDataHolderAccessor accessor(this);

    if (!mInfo->mIsPlayerHunting) {
        mModeTimer->enableTimer();
        mModeLayout->showHiding();

        if (accessor.mData && accessor.mData->mGameDataFile) {
            accessor.mData->mGameDataFile->setKidsMode(true);
        }

    } else {
        mModeTimer->disableTimer();
        mModeLayout->showSeeking();
    }


    GameModeBase::begin();
}

void ManHuntMode::end() {

    mModeLayout->tryEnd();

    mModeTimer->disableTimer();

    mInvulnTime = 0.0f;

    GameModeBase::end();
}

void ManHuntMode::update() {

    PlayerActorBase* playerBase = rs::getPlayerActor(mCurScene);

    bool isYukimaru = !playerBase->getPlayerInfo(); // if PlayerInfo is a nullptr, that means we're dealing with the bound bowl racer

    if (mIsFirstFrame) {

        if (mInfo->mIsUseGravityCam && mTicket) {
            al::startCamera(mCurScene, mTicket, -1);
        }

        mIsFirstFrame = false;
    }

    // Inside ManHuntMode::update()
PlayerActorHakoniwa* player = dynamic_cast<PlayerActorHakoniwa*>(rs::getPlayerActor(mCurScene));
if (player && player->mPlayerAnimator) {
    if (player->mPlayerAnimator->isAnim("CatchKoopaCap")) {
        // Grant invincibility while catching cap
        mIsCapInvincible = true;
        mCapInvulnTimer = 0.0f; // reset timer
    }
    else if (player->mPlayerAnimator->isAnim("KoopaCapPunchFinishL") ||
             player->mPlayerAnimator->isAnim("KoopaCapPunchFinishR")) {
        // If not already counting down, start 5s grace
        if (mCapInvulnTimer <= 0.0f) {
            mCapInvulnTimer = 2.0f;
        }
    }
}

// Tick down invincibility timer
if (mCapInvulnTimer > 0.0f) {
    mCapInvulnTimer -= Time::deltaTime; // use your custom delta time
    if (mCapInvulnTimer <= 0.0f) {
        mIsCapInvincible = false; // remove invincibility after 5s
    }
}


    ShineTowerRocket* odyssey = rs::tryGetShineTowerRocketFromDemoDirector((al::LiveActor*)playerBase);
    if (odyssey) {
        if (GameModeManager::instance()->isModeAndActive(GameMode::MANHUNT)) {
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

        al::invalidateClipping((al::LiveActor*)odyssey);
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
        } else if (al::isPadTriggerZL(-1)) {
            if (al::isPadTriggerLeft(-1)) {
                killMainPlayer(((PlayerActorHakoniwa*)playerBase));
            }
        }
    }

    if (al::isPadTriggerUp(-1) && !al::isPadHoldZL(-1))
    {
        GameDataHolderAccessor accessor(this);
        mInfo->mIsPlayerHunting = !mInfo->mIsPlayerHunting;

        mModeTimer->toggleTimer();

        if(!mInfo->mIsPlayerHunting) {
            mInvulnTime = 0;
            mModeLayout->showHiding();

            if (accessor.mData && accessor.mData->mGameDataFile) {
            accessor.mData->mGameDataFile->setKidsMode(true);
            }

        } else {
            mModeLayout->showSeeking();

            if (accessor.mData && accessor.mData->mGameDataFile) {
            accessor.mData->mGameDataFile->setKidsMode(false);
            }
            
        }

        Client::sendTagInfPacket();
    }

    mInfo->mHidingTime = mModeTimer->getTime();
}

// Cap Damage

bool ManHuntMode::handleCapAttack(al::HitSensor* sender, al::HitSensor* receiver, PuppetInfo* attackerInfo) {
    if (!attackerInfo) {
        Logger::log("ManHunt cap attack blocked: no attacker info\n");
        return false;
    }

    if (mIsCapInvincible) {
    Logger::log("Cap attack blocked: player invincible (KoopaCap)\n");
    return false;
}


    // Get the attacking puppet's hunting status
    bool attackerIsHunting = !attackerInfo->manhuntIsRunning(); // manhuntIsRunning returns true for hiders

    // Find receiver info
    al::LiveActor* receiverActor = nullptr;
    bool receiverIsHunting = false;
    bool receiverIsInHack = false;
    const char* hackName = nullptr;
    
    if (!findReceiverPlayerInfo(receiver, receiverActor, receiverIsHunting, receiverIsInHack, hackName)) {
        Logger::log("ManHunt cap attack blocked: couldn't identify receiver as player\n");
        return false;
    }

    // Tank immunity check
    if (receiverIsInHack && hackName && strcmp(hackName, "Tank") == 0) {
        Logger::log("ManHunt cap attack blocked: Tank immunity\n");
        return false;
    }

    // Safe zone check
    if (!checkSafeZone(attackerInfo->playerPos, receiverActor, attackerIsHunting, receiverIsHunting)) {
        return false;
    }

    // Only allow damage if players are on different teams
    if (attackerIsHunting != receiverIsHunting) {
        performAttack(sender, receiver, receiverActor, receiverIsInHack);
        
        Logger::log("ManHunt cap hit: %s hit %s%s\n", 
                   attackerIsHunting ? "Hunter" : "Hider",
                   receiverIsHunting ? "Hunter" : "Hider",
                   receiverIsInHack ? " (in hack)" : "");
        return true;
    } else {
        Logger::log("ManHunt cap hit blocked: same team (%s) cannot attack each other\n",
                   attackerIsHunting ? "Hunters" : "Hiders");
        return false;
    }
}

bool ManHuntMode::isPlayerHuntingByInfo(PuppetInfo* puppetInfo) const {
    return !puppetInfo->manhuntIsRunning(); // manhuntIsRunning returns true for hiders
}

bool ManHuntMode::findReceiverPlayerInfo(al::HitSensor* receiver, al::LiveActor*& receiverActor, 
                                         bool& isHunting, bool& isInHack, const char*& hackName) {
    receiverActor = al::getSensorHost(receiver);
    isHunting = false;
    isInHack = false;
    hackName = nullptr;
    
    // Get current scene and local player
    auto* curSeq = (HakoniwaSequence*) GameSystemFunction::getGameSystem()->mSequence;
    if (!curSeq || !curSeq->curScene) return false;
    
    StageScene* stageScene = (StageScene*) curSeq->curScene;
    al::PlayerHolder* pHolder = al::getScenePlayerHolder(stageScene);
    PlayerActorBase* playerBase = al::tryGetPlayerActor(pHolder, 0);
    auto* localPlayer = dynamic_cast<PlayerActorHakoniwa*>(playerBase);
    
    // Check if receiver is the local player (direct hit)
    if (al::isSensorPlayer(receiver)) {
        if (receiverActor == localPlayer) {
            isHunting = mInfo->mIsPlayerHunting;
            
            // Check if player is in a hack
            if (localPlayer && localPlayer->mHackKeeper && localPlayer->mHackKeeper->currentHackActor) {
                isInHack = true;
                hackName = localPlayer->mHackKeeper->getCurrentHackName();
            }
            return true;
        }
    }
    
    // Check if receiver is a hacked actor controlled by local player
    if (!al::isSensorPlayer(receiver)) {
        if (localPlayer && localPlayer->mHackKeeper && localPlayer->mHackKeeper->currentHackActor) {
            if (localPlayer->mHackKeeper->currentHackActor == receiverActor) {
                isHunting = mInfo->mIsPlayerHunting;
                isInHack = true;
                hackName = localPlayer->mHackKeeper->getCurrentHackName();
                return true;
            }
        }
    }
    
    // Check puppet players
    if (mPuppetHolder) {
        for (size_t i = 0; i < (size_t)mPuppetHolder->getSize(); i++) {
            PuppetInfo* puppetInfo = Client::getPuppetInfo(i);
            if (!puppetInfo || !puppetInfo->isConnected || !puppetInfo->isInSameStage) {
                continue;
            }
            
            PuppetActor* puppetActor = mPuppetHolder->getPuppetActor(i);
            
            // Check normal puppet actor (direct hit)
            if (al::isSensorPlayer(receiver) && puppetActor && puppetActor == receiverActor) {
                isHunting = isPlayerHuntingByInfo(puppetInfo);
                return true;
            }
            
            // Check if receiver is a hacked actor controlled by this puppet
            if (!al::isSensorPlayer(receiver)) {
                // Simplified approach - check if puppet is close to the receiver actor
                // You may need to enhance PuppetInfo to track hack state properly
                if (puppetActor && al::calcDistance(puppetActor, receiverActor) < 200.0f) {
                    isHunting = isPlayerHuntingByInfo(puppetInfo);
                    isInHack = true;
                    // Note: hackName remains nullptr as we don't have puppet hack info
                    return true;
                }
            }
        }
    }
    
    return false;
}

bool ManHuntMode::checkSafeZone(const sead::Vector3f& attackerPos, al::LiveActor* receiverActor, 
                                bool attackerIsHunting, bool receiverIsHunting) {
    // Get Odyssey position for safe zone checking
    al::PlayerHolder* pHolder = al::getScenePlayerHolder(mCurScene);
    PlayerActorBase* playerBase = al::tryGetPlayerActor(pHolder, 0);
    if (!playerBase) return true; // If no player, allow attack
    
    ShineTowerRocket* odyssey = rs::tryGetShineTowerRocketFromDemoDirector((al::LiveActor*)playerBase);
    if (!odyssey) return true; // If no Odyssey, allow attack
    
    sead::Vector3f odysseyPos = al::getTrans((al::LiveActor*)odyssey);
    const f32 safeZoneDistance = 2045.0f;

    // Check if attacker is in safe zone (only hiders get protection)
    bool attackerInSafeZone = false;
    if (!attackerIsHunting) {
        // Horizontal distance only (ignore Y)
        f32 dx = attackerPos.x - odysseyPos.x;
        f32 dz = attackerPos.z - odysseyPos.z;
        f32 attackerDistance = std::sqrt(dx*dx + dz*dz);
        attackerInSafeZone = (attackerDistance < safeZoneDistance);
    }

    // Check if receiver is in safe zone (only hiders get protection)
    bool receiverInSafeZone = false;
    if (!receiverIsHunting) {
        f32 receiverDistance = al::calcDistanceH(receiverActor, (al::LiveActor*)odyssey);
        receiverInSafeZone = (receiverDistance < safeZoneDistance);
    }

    // Block attack if either player is a hider in the safe zone
    if (attackerInSafeZone || receiverInSafeZone) {
        Logger::log("ManHunt cap attack blocked: %s in safe zone\n", 
                   attackerInSafeZone ? "Attacker" : "Receiver");
        return false;
    }
    
    return true;
}


void ManHuntMode::performAttack(al::HitSensor* sender, al::HitSensor* receiver, 
                                al::LiveActor* receiverActor, bool receiverIsInHack) {
    // For hack actors, damage both the hack actor and the controlling player
    if (receiverIsInHack && !al::isSensorPlayer(receiver)) {
        // Damage the hack actor
        al::sendMsgEnemyAttack(receiver, sender);
        
        // Also damage the controlling player (local player only for now)
        al::PlayerHolder* pHolder = al::getScenePlayerHolder(mCurScene);
        PlayerActorBase* playerBase = al::tryGetPlayerActor(pHolder, 0);
        auto* localPlayer = dynamic_cast<PlayerActorHakoniwa*>(playerBase);
        
        if (localPlayer && localPlayer->mHackKeeper && 
            localPlayer->mHackKeeper->currentHackActor == receiverActor) {
            al::HitSensor* playerSensor = al::getHitSensor(localPlayer, "Body");
            if (playerSensor) {
                al::sendMsgEnemyAttack(playerSensor, sender);
            }
        }
    } else {
        // Direct player hit
        al::sendMsgEnemyAttack(receiver, sender);
    }
}

// Hooks

void stageSceneHook() {
    StageScene *stageScene;
    __asm("MOV %[result], X0" : [result] "=r"(stageScene));
    isInGame = true;

    auto *pHolder = al::getScenePlayerHolder(stageScene);
    auto *player = (PlayerActorHakoniwa*)al::tryGetPlayerActor(pHolder, 0);
    PlayerActorBase*  playerBase = al::tryGetPlayerActor(pHolder, 0);
    if (!player) return;

    GameDataHolderWriter writer(stageScene->mHolder);
    GameDataFunction::enableCap(writer);
    GameDataFunction::talkCapNearHomeInWaterfall(player);

    if (!GameModeManager::instance()->isModeAndActive(GameMode::MANHUNT)) {
        ShineTowerRocket* odyssey = rs::tryGetShineTowerRocketFromDemoDirector((al::LiveActor*)playerBase);
        if (odyssey) {
            al::tryDeleteEffect((al::LiveActor*)odyssey, "Special1WorldHomeGKBarrier");
        }
    }


}

// ManHunt bool and stuff

namespace al {
    bool trySyncStageSwitchAppearAndKill(LiveActor*);
    const char* getModelName(const LiveActor* actor);
    void startNerveAction(LiveActor*, const char*);
}

void barrierAppearHook(al::LiveActor* thisPtr, const char* actionName) {
    if (!GameModeManager::instance()->isModeAndActive(GameMode::MANHUNT)) {
        return; // Disable functionality if not in MANHUNT mode
    }

    if (al::isEqualString(GameDataFunction::getCurrentStageName(thisPtr), "SkyWorldHomeStage") && 
        al::calcDistanceH(thisPtr, sead::Vector3f{5722.f, 29000.f, -41583.f}) < 200) {
        // thisPtr->kill();
        al::startNerveAction(thisPtr, "Disappear");
    } else {
        al::startNerveAction(thisPtr, actionName);
    }
}


bool compassAlwaysVisible(GameDataHolderAccessor accessor) {
    return GameModeManager::instance()->isModeAndActive(GameMode::MANHUNT);
}
