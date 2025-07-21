#include "actors/PuppetCapActor.h"
#include "al/sensor/HitSensor.h"
#include "al/util.hpp"
#include "al/util/MathUtil.h"
#include "logger.hpp"
#include "math/seadVector.h"
#include "rs/util/SensorUtil.h"
#include "al/util/SensorUtil.h"
#include "server/gamemode/GameModeManager.hpp"
#include "server/gamemode/GameModeBase.hpp"

#include "server/manhunt/ManHuntMode.hpp"
#include "server/Client.hpp"
#include "game/HakoniwaSequence/HakoniwaSequence.h"
#include "game/System/GameSystem.h"
#include "server/DeltaTime.hpp"

PuppetCapActor::PuppetCapActor(const char* name) : al::LiveActor(name) {
    mIsInvincible = false;
    mInvincibleTimer = 0.0f;
}

void PuppetCapActor::init(al::ActorInitInfo const &initInfo) {

    sead::FixedSafeString<0x20> capModelName;

    PlayerFunction::createCapModelName(&capModelName, tryGetPuppetCapName(mInfo));

    PlayerFunction::initCapModelActorDemo(this, initInfo, capModelName.cstr());

    initHitSensor(2);

    al::addHitSensor(this, initInfo, "Push", SensorType::MapObjSimple, 60.0f, 8,
                     sead::Vector3f::zero);

    al::addHitSensor(this, initInfo, "Attack", SensorType::EnemyAttack, 300.0f, 8,
                     sead::Vector3f::zero);

    al::hideSilhouetteModelIfShow(this);

    al::initExecutorModelUpdate(this, initInfo);

    mJointKeeper = new HackCapJointControlKeeper();

    mJointKeeper->initCapJointControl(this);

    makeActorDead();
}

void PuppetCapActor::initAfterPlacement() {
    al::LiveActor::initAfterPlacement();
}

void PuppetCapActor::initOnline(PuppetInfo *pupInfo) {
    mInfo = pupInfo;
}

void PuppetCapActor::movement() {
    al::LiveActor::movement();
}

void PuppetCapActor::control() {
    if(mInfo->capAnim) {
        startAction(mInfo->capAnim);
    }

    auto* curSeq = (HakoniwaSequence*) GameSystemFunction::getGameSystem()->mSequence;
    if (!curSeq || !curSeq->curScene) return;
    StageScene* stageScene = (StageScene*) curSeq->curScene;
    ManHuntMode* hnsMode = GameModeManager::instance()->getMode<ManHuntMode>();

    al::PlayerHolder* pHolder = al::getScenePlayerHolder(stageScene);
    PlayerActorBase* playerBase = al::tryGetPlayerActor(pHolder, 0);
    auto* player = dynamic_cast<PlayerActorHakoniwa*>(playerBase);
    if (!player) return;

    // Invincibility logic for animations
    if (player->mPlayerAnimator->isAnim("CatchKoopaCap")) {
        mIsInvincible = true;
        mInvincibleTimer = 0.0f;
    } 
    else if (player->mPlayerAnimator->isAnim("KoopaCapPunchFinishL") ||
             player->mPlayerAnimator->isAnim("KoopaCapPunchFinishR")) 
    {
        if (mIsInvincible && mInvincibleTimer <= 0.0f) {
            mInvincibleTimer = 0.85f;
        }
    } 
    else if (mInvincibleTimer > 0.0f) {
        mInvincibleTimer -= Time::deltaTime;
        if (mInvincibleTimer <= 0.0f) {
            mIsInvincible = false;
            mInvincibleTimer = 0.0f;
        }
    }

    sead::Vector3f *cPos = al::getTransPtr(this);

    if(*cPos != mInfo->capPos) 
    {
        al::lerpVec(cPos, *cPos, mInfo->capPos, 0.45);
    }

    mJointKeeper->mJointRot.x = al::lerpValue(mJointKeeper->mJointRot.x, mInfo->capRot.x, 0.85);
    mJointKeeper->mJointRot.y = al::lerpValue(mJointKeeper->mJointRot.y, mInfo->capRot.y, 0.85);
    mJointKeeper->mJointRot.z = al::lerpValue(mJointKeeper->mJointRot.z, mInfo->capRot.z, 0.85);
    mJointKeeper->mSkew = al::lerpValue(mJointKeeper->mSkew, mInfo->capRot.w, 0.85);
}

void PuppetCapActor::update() {
    al::LiveActor::calcAnim();
    al::LiveActor::movement();
}

void PuppetCapActor::attackSensor(al::HitSensor* sender, al::HitSensor* receiver) {
    
    auto* curSeq = (HakoniwaSequence*) GameSystemFunction::getGameSystem()->mSequence;
    if (!curSeq || !curSeq->curScene) return;
    StageScene* stageScene = (StageScene*) curSeq->curScene;
    ManHuntMode* hnsMode = GameModeManager::instance()->getMode<ManHuntMode>();

    al::PlayerHolder* pHolder = al::getScenePlayerHolder(stageScene);
    PlayerActorBase* playerBase = al::tryGetPlayerActor(pHolder, 0);
    auto* player = dynamic_cast<PlayerActorHakoniwa*>(playerBase);
    if (!player) return;
    
    if (al::isSensorPlayer(receiver) && al::isSensorName(sender, "Push")) {
        rs::sendMsgPushToPlayer(receiver, sender);
    }

    // Handle cap-only damage for seeker vs hider combat
    if (al::isSensorPlayer(receiver) && al::isSensorName(sender, "Attack")) {
        // Check if we're in Hide and Seek mode
        if (GameModeManager::instance()->isModeAndActive(GameMode::MANHUNT)) {
            ManHuntMode* hnsMode = GameModeManager::instance()->getMode<ManHuntMode>();
            
        auto* targetPlayerHako = dynamic_cast<PlayerActorHakoniwa*>(player);
        if (targetPlayerHako && targetPlayerHako->mHackKeeper && 
            targetPlayerHako->mHackKeeper->currentHackActor) {
            const char* hackName = targetPlayerHako->mHackKeeper->getCurrentHackName();
            if (hackName && strcmp(hackName, "Tank") == 0) {
                return;
            }
        }


            if (mIsInvincible) return;
            if (hnsMode && mInfo) {
                // Get cap owner's role
                bool capOwnerIsSeeker = mInfo->isIt;
                
                // Get target's role - assume if it's hitting local player
                bool targetIsSeeker = hnsMode->isPlayerHunting();
                
                // Simple cross-team validation
                // Only allow damage if roles are different (seeker vs hider or hider vs seeker)
                if (capOwnerIsSeeker != targetIsSeeker) {
                    
                    // If cap owner is a hider, check if they're in safe zone
                    if (!capOwnerIsSeeker) { // Cap owner is hider/runner
                        // Find the cap owner's puppet by iterating through all puppets
                        PuppetHolder* puppetHolder = Client::getPuppetHolder();
                        if (puppetHolder) {
                            for (int i = 0; i < puppetHolder->getSize(); i++) {
                                PuppetActor* puppet = puppetHolder->getPuppetActor(i);
                                if (puppet && puppet->getInfo() && 
                                    puppet->getInfo()->playerID == mInfo->playerID) {
                                    // Found the cap owner's puppet
                                    if (hnsMode->isPlayerInSafeZone(puppet)) {
                                        return; // Cap owner (hider) is in safe zone, can't deal damage
                                    }
                                    break;
                                }
                            }
                        }
                    }
                    
                    // If target is a hider, check if they're in safe zone
                    if (!targetIsSeeker) { // Target is hider/runner
                        al::LiveActor* targetActor = al::getSensorHost(receiver);
                        if (targetActor && hnsMode->isPlayerInSafeZone(targetActor)) {
                            return; // Hider is in safe zone, no damage
                        }
                    }
                    
                    // Send enemy attack message for cap damage
                    al::sendMsgEnemyAttack(receiver, sender);
                }
            }
        }
    }
}

bool PuppetCapActor::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender,
                             al::HitSensor* receiver) {


    if (al::isMsgPlayerDisregard(msg)) {
        return true;
    }

    if (rs::isMsgPlayerCapTouchJump(msg)) {
        return true;
    }

    if (rs::isMsgPlayerCapTrample(msg)) {
        rs::requestHitReactionToAttacker(msg, receiver, *al::getSensorPos(sender));
        return true;
    }

    return false;
}

void PuppetCapActor::startAction(const char *actName) {
    if(al::tryStartActionIfNotPlaying(this, actName)) {
        const char *curActName = al::getActionName(this);
        if(curActName) {
            if(al::isSklAnimExist(this, curActName)) {
                al::clearSklAnimInterpole(this);
            }
        }
    }
}