#include "actors/PuppetCapActor.h"

#include "al/util.hpp"
#include "al/util/MathUtil.h"
#include "al/util/SensorUtil.h"

#include "game/Player/PlayerFunction.h"

#include "rs/util/SensorUtil.h"

#include "sead/math/seadVector.h"

#include "server/gamemode/GameModeManager.hpp"

#include "server/hns/HideAndSeekMode.hpp"  
#include "server/Client.hpp"

PuppetCapActor::PuppetCapActor(const char* name) : al::LiveActor(name) {}

void PuppetCapActor::init(al::ActorInitInfo const& initInfo) {
    sead::FixedSafeString<0x20> capModelName;

    PlayerFunction::createCapModelName(&capModelName, tryGetPuppetCapName(mInfo));

    PlayerFunction::initCapModelActorDemo(this, initInfo, capModelName.cstr());

    initHitSensor(2);

    al::addHitSensor(this, initInfo, "Push", SensorType::MapObjSimple, 60.0f, 8, sead::Vector3f::zero);

    al::addHitSensor(this, initInfo, "Attack", SensorType::EnemyAttack, 300.0f, 8, sead::Vector3f::zero);

    al::hideSilhouetteModelIfShow(this);

    al::initExecutorModelUpdate(this, initInfo);

    mJointKeeper = new HackCapJointControlKeeper();

    mJointKeeper->initCapJointControl(this);

    makeActorDead();
}

void PuppetCapActor::initAfterPlacement() {
    al::LiveActor::initAfterPlacement();
}

void PuppetCapActor::initOnline(PuppetInfo* pupInfo) {
    mInfo = pupInfo;
}

void PuppetCapActor::movement() {
    al::LiveActor::movement();
}

void PuppetCapActor::control() {
    if (mInfo->capAnim) {
        startAction(mInfo->capAnim);
    }

    sead::Vector3f* cPos = al::getTransPtr(this);

    if (*cPos != mInfo->capPos) {
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

    if (GameModeManager::instance()->isModeAndActive(GameMode::HIDEANDSEEK)) {
        al::LiveActor* targetPlayer = nullptr;
        
        // Check direct player hit or captured actor hit
        if (al::isSensorPlayer(receiver)) {
            targetPlayer = al::getSensorHost(receiver);
        } else {
            // Check if this is a captured/hacked actor
            auto* receiverHost = al::getSensorHost(receiver);
            auto* player = (PlayerActorHakoniwa*) al::getPlayerActor(receiverHost, 0);
            
            if (player && player->mHackKeeper && 
                player->mHackKeeper->currentHackActor && 
                player->mHackKeeper->currentHackActor == receiverHost) {
                targetPlayer = player;
            }
        }
        
        if (!targetPlayer) return;

            // Get the Hide and Seek mode instance
            HideAndSeekMode* hnsMode = GameModeManager::instance()->getMode<HideAndSeekMode>();
            if (!hnsMode || !mInfo) return;

            // CRITICAL: Check barrier protection first, before any role checks
            if (hnsMode->isPlayerNearOdysseyBarrier(targetPlayer)) {
                return; // Absolute protection - no damage allowed when near barrier
            }

            // Also check if the cap OWNER is near barrier (in case barrier affects both players)
            if (hnsMode->isPlayerNearOdysseyBarrier(this)) {
                return; // No damage if cap owner is also near barrier
            }



            // Get target player's position for puppet matching
            sead::Vector3f targetPos = al::getTrans(targetPlayer);

            // Find the target player's info with improved matching
            PuppetInfo* targetInfo = nullptr;
            float closestDistSq = 10000.0f; // Start with max distance
            
            for (size_t i = 0; i < Client::getMaxPlayerCount(); i++) {
                PuppetInfo* puppet = Client::getPuppetInfo(i);
                if (!puppet || !puppet->isConnected) continue;

                float distSq = vecDistanceSq(puppet->playerPos, targetPos);
                if (distSq < closestDistSq) {
                    closestDistSq = distSq;
                    targetInfo = puppet;
                }
            }

            // Verify we found a close enough match (within reasonable distance)
            if (targetInfo && closestDistSq > 2500.0f) { // ~50 units squared
                targetInfo = nullptr; // Distance too far, likely not a match
            }

            // Determine if target is main player and get their role
            bool targetIsHiding = false;
            bool targetIsSeeking = false;
            
            if (!targetInfo) {
                // Target is likely the main player - get role from mode
                targetIsHiding = hnsMode->isPlayerHiding();
                targetIsSeeking = hnsMode->isPlayerSeeking();
            } else {
                // Target is a puppet - get role from PuppetInfo
                targetIsHiding = targetInfo->hnsIsHiding();
                targetIsSeeking = targetInfo->hnsIsSeeking();
            }

            // Only allow damage if roles are opposite AND neither is near barrier
            bool canDealDamage = false;
            
            if (mInfo->hnsIsSeeking() && targetIsHiding) {
                canDealDamage = true;
            } else if (mInfo->hnsIsHiding() && targetIsSeeking) {
                canDealDamage = true;
            }

            // Final barrier check before dealing damage (double safety)
            if (canDealDamage && !hnsMode->isPlayerNearOdysseyBarrier(targetPlayer)) {
                al::sendMsgEnemyAttack(receiver, sender);
            }
            
            return;
        }
    }

    if (!GameModeManager::hasCappyCollision()) {
        return;
    }

    // Original logic for non-Hide and Seek modes
    if (al::isSensorPlayer(receiver) && al::isSensorName(sender, "Push")) {
        rs::sendMsgPushToPlayer(receiver, sender);
    }
}

bool PuppetCapActor::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (!GameModeManager::hasCappyBounce()) {
        return false;
    }

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

void PuppetCapActor::startAction(const char* actName) {
    if (al::tryStartActionIfNotPlaying(this, actName)) {
        const char* curActName = al::getActionName(this);
        if (curActName) {
            if (al::isSklAnimExist(this, curActName)) {
                al::clearSklAnimInterpole(this);
            }
        }
    }
}
