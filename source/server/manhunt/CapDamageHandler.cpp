#include "server/manhunt/CapDamageHandler.h"
#include "al/util/SensorUtil.h"
#include "game/HakoniwaSequence/HakoniwaSequence.h"
#include "game/System/GameSystem.h"
#include "server/gamemode/GameModeManager.hpp"
#include "server/manhunt/ManHuntMode.hpp"
#include "server/Client.hpp"

bool CapDamageHandler::isInManHuntMode() {
    return GameModeManager::instance()->isModeAndActive(GameMode::MANHUNT);
}

bool CapDamageHandler::isTankCapture(const char* hackName) {
    return hackName && strcmp(hackName, "Tank") == 0;
}

bool CapDamageHandler::isValidDamageScenario(PuppetInfo* capOwnerInfo, bool& capOwnerIsSeeker, bool& targetIsSeeker) {
    if (!capOwnerInfo) return false;
    
    ManHuntMode* hnsMode = GameModeManager::instance()->getMode<ManHuntMode>();
    if (!hnsMode) return false;
    
    capOwnerIsSeeker = capOwnerInfo->isIt;
    targetIsSeeker = hnsMode->isPlayerHunting();
    
    // Only allow damage between different teams
    return capOwnerIsSeeker != targetIsSeeker;
}

bool CapDamageHandler::isCapOwnerInSafeZone(PuppetInfo* capOwnerInfo) {
    if (!capOwnerInfo) return false;
    
    ManHuntMode* hnsMode = GameModeManager::instance()->getMode<ManHuntMode>();
    if (!hnsMode) return false;
    
    PuppetHolder* puppetHolder = Client::getPuppetHolder();
    if (!puppetHolder) return false;
    
    for (int i = 0; i < puppetHolder->getSize(); i++) {
        PuppetActor* puppet = puppetHolder->getPuppetActor(i);
        if (puppet && puppet->getInfo() && 
            puppet->getInfo()->playerID == capOwnerInfo->playerID) {
            return hnsMode->isPlayerInSafeZone(puppet);
        }
    }
    return false;
}

bool CapDamageHandler::isTargetInSafeZone(al::LiveActor* targetActor, bool targetIsSeeker) {
    if (!targetActor || targetIsSeeker) return false; // Seekers don't get safe zone protection
    
    ManHuntMode* hnsMode = GameModeManager::instance()->getMode<ManHuntMode>();
    return hnsMode && hnsMode->isPlayerInSafeZone(targetActor);
}

void CapDamageHandler::sendDamageToPlayer(al::HitSensor* receiver, al::HitSensor* sender) {
    al::sendMsgEnemyAttack(receiver, sender);
}

void CapDamageHandler::sendDamageToHackActor(al::HitSensor* receiver, al::HitSensor* sender, al::LiveActor* hackActor) {
    // Damage the hack actor
    al::sendMsgEnemyAttack(receiver, sender);
    
    // Also damage the controlling player
    auto* curSeq = (HakoniwaSequence*) GameSystemFunction::getGameSystem()->mSequence;
    if (!curSeq || !curSeq->curScene) return;
    
    StageScene* stageScene = (StageScene*) curSeq->curScene;
    al::PlayerHolder* pHolder = al::getScenePlayerHolder(stageScene);
    PlayerActorBase* playerBase = al::tryGetPlayerActor(pHolder, 0);
    auto* player = dynamic_cast<PlayerActorHakoniwa*>(playerBase);
    
    if (player) {
        al::HitSensor* playerSensor = al::getHitSensor(player, "Body");
        if (playerSensor) {
            al::sendMsgEnemyAttack(playerSensor, sender);
        }
    }
}

void CapDamageHandler::handleCapDamage(al::HitSensor* sender, al::HitSensor* receiver, 
                                      PuppetInfo* capOwnerInfo, bool isInvincible) {
    if (!al::isSensorName(sender, "Attack") || !isInManHuntMode() || isInvincible) {
        return;
    }
    
    bool capOwnerIsSeeker, targetIsSeeker;
    if (!isValidDamageScenario(capOwnerInfo, capOwnerIsSeeker, targetIsSeeker)) {
        return;
    }
    
    // Handle direct player damage
    if (al::isSensorPlayer(receiver)) {
        // Get current player for tank check
        auto* curSeq = (HakoniwaSequence*) GameSystemFunction::getGameSystem()->mSequence;
        if (!curSeq || !curSeq->curScene) return;
        
        StageScene* stageScene = (StageScene*) curSeq->curScene;
        al::PlayerHolder* pHolder = al::getScenePlayerHolder(stageScene);
        PlayerActorBase* playerBase = al::tryGetPlayerActor(pHolder, 0);
        auto* player = dynamic_cast<PlayerActorHakoniwa*>(playerBase);
        
        if (player && player->mHackKeeper && player->mHackKeeper->currentHackActor) {
            const char* hackName = player->mHackKeeper->getCurrentHackName();
            if (isTankCapture(hackName)) return;
        }
        
        // Check safe zones
        if (!capOwnerIsSeeker && isCapOwnerInSafeZone(capOwnerInfo)) return;
        
        al::LiveActor* targetActor = al::getSensorHost(receiver);
        if (isTargetInSafeZone(targetActor, targetIsSeeker)) return;
        
        sendDamageToPlayer(receiver, sender);
        return;
    }
    
    // Handle hack actor damage
    if (!al::isSensorPlayer(receiver)) {
        al::LiveActor* hackActor = al::getSensorHost(receiver);
        if (!hackActor) return;
        
        // Verify this is the local player's current hack
        auto* curSeq = (HakoniwaSequence*) GameSystemFunction::getGameSystem()->mSequence;
        if (!curSeq || !curSeq->curScene) return;
        
        StageScene* stageScene = (StageScene*) curSeq->curScene;
        al::PlayerHolder* pHolder = al::getScenePlayerHolder(stageScene);
        PlayerActorBase* playerBase = al::tryGetPlayerActor(pHolder, 0);
        auto* player = dynamic_cast<PlayerActorHakoniwa*>(playerBase);
        
        if (!player || !player->mHackKeeper || !player->mHackKeeper->currentHackActor) return;
        if (player->mHackKeeper->currentHackActor != hackActor) return;
        
        // Tank immunity
        const char* hackName = player->mHackKeeper->getCurrentHackName();
        if (isTankCapture(hackName)) return;
        
        // Check safe zones
        if (!capOwnerIsSeeker && isCapOwnerInSafeZone(capOwnerInfo)) return;
        if (isTargetInSafeZone(hackActor, targetIsSeeker)) return;
        
        sendDamageToHackActor(receiver, sender, hackActor);
    }
}