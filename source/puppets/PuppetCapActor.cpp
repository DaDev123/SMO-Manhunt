#include "actors/PuppetCapActor.h"

#include "al/util.hpp"
#include "rs/util.hpp"
#include "al/util/MathUtil.h"
#include "al/util/SensorUtil.h"

#include "game/Player/PlayerFunction.h"

#include "rs/util/SensorUtil.h"

#include "sead/math/seadVector.h"

#include "server/gamemode/GameModeManager.hpp"

#include "server/manhunt/ManHuntMode.hpp"  
#include "server/Client.hpp"
#include "game/HakoniwaSequence/HakoniwaSequence.h"
#include "game/System/GameSystem.h"
#include "server/DeltaTime.hpp"

PuppetCapActor::PuppetCapActor(const char* name) : al::LiveActor(name) {
    mIsInvincible = false;
    mInvincibleTimer = 0.0f;
    mWasNearBarrier = false;
}

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
    Time::calcTime();

    if (mInfo->capAnim) {
        startAction(mInfo->capAnim);
    }

    auto* curSeq = (HakoniwaSequence*) GameSystemFunction::getGameSystem()->mSequence;
    if (!curSeq || !curSeq->curScene) return;
    StageScene* stageScene = (StageScene*) curSeq->curScene;
    ManHuntMode* manhuntMode = GameModeManager::instance()->getMode<ManHuntMode>();

    al::PlayerHolder* pHolder = al::getScenePlayerHolder(stageScene);
    PlayerActorBase* playerBase = al::tryGetPlayerActor(pHolder, 0);
    auto* player = dynamic_cast<PlayerActorHakoniwa*>(playerBase);
    if (!player) return;

    if (player && manhuntMode && mInfo) {
        bool isNearBarrier = manhuntMode->isPlayerNearOdysseyBarrier(player);

        if (mWasNearBarrier && !isNearBarrier) {
            mIsInvincible = true;
            mInvincibleTimer = 0.5f;
        }

        mWasNearBarrier = isNearBarrier;
    }

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

    // Lerp position/rotation to sync puppet with owner
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
    ManHuntMode* manhuntMode = GameModeManager::instance()->getMode<ManHuntMode>();
    if (mIsInvincible) return;

    if (GameModeManager::instance()->isModeAndActive(GameMode::MANHUNT)) {
        al::LiveActor* targetPlayer = nullptr;

        if (al::isSensorPlayer(receiver)) {
            targetPlayer = al::getSensorHost(receiver);
        } else {
            auto* receiverHost = al::getSensorHost(receiver);
            if (receiverHost) {
                auto* playerActor = al::getPlayerActor(receiverHost, 0);
                if (playerActor) {
                    auto* player = dynamic_cast<PlayerActorHakoniwa*>(playerActor);
                    if (player && player->mHackKeeper && player->mHackKeeper->currentHackActor == receiverHost) {
                        targetPlayer = player;
                    }
                }
            }
        }

        if (!targetPlayer) return;

        auto* targetPlayerHako = dynamic_cast<PlayerActorHakoniwa*>(targetPlayer);
        if (targetPlayerHako && targetPlayerHako->mHackKeeper && 
            targetPlayerHako->mHackKeeper->currentHackActor) {
            const char* hackName = targetPlayerHako->mHackKeeper->getCurrentHackName();
            if (hackName && strcmp(hackName, "Tank") == 0) {
                return;
            }
        }

        if (!manhuntMode || !mInfo) return;

        if (manhuntMode->isPlayerNearOdysseyBarrier(targetPlayer)) return;
        if (manhuntMode->isPlayerNearOdysseyBarrier(this)) return;

        if (!mCapDamageEnabled) return;

        sead::Vector3f targetPos = al::getTrans(targetPlayer);

        PuppetInfo* targetInfo = nullptr;
        float closestDistSq = 10000.0f;

        for (int i = 0; i < Client::getMaxPlayerCount(); i++) {
            PuppetInfo* puppet = Client::getPuppetInfo(i);
            if (!puppet || !puppet->isConnected) continue;

            float distSq = vecDistanceSq(puppet->playerPos, targetPos);
            if (distSq < closestDistSq) {
                closestDistSq = distSq;
                targetInfo = puppet;
            }
        }

        if (targetInfo && closestDistSq > 2500.0f) {
            targetInfo = nullptr;
        }

        bool targetIsHiding = false;
        bool targetIsSeeking = false;

        if (!targetInfo) {
            targetIsHiding = manhuntMode->isPlayerRunning();
            targetIsSeeking = manhuntMode->isPlayerHunting();
        } else {
            targetIsHiding = targetInfo->manhuntIsRunning();
            targetIsSeeking = targetInfo->manhuntIsHunting();
        }

        bool canDealDamage = false;

        if (mInfo->manhuntIsHunting() && targetIsHiding) {
            canDealDamage = true;
        } else if (mInfo->manhuntIsRunning() && targetIsSeeking) {
            canDealDamage = true;
        }

        if (canDealDamage && !manhuntMode->isPlayerNearOdysseyBarrier(targetPlayer)) {
            al::sendMsgEnemyAttack(receiver, sender);
        }

        return;
    }

    if (!GameModeManager::hasCappyCollision()) return;

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
        if (curActName && al::isSklAnimExist(this, curActName)) {
            al::clearSklAnimInterpole(this);
        }
    }
}
