#pragma once

#include "al/sensor/HitSensor.h"
#include "puppets/PuppetInfo.h"

class CapDamageHandler {
private:
    static bool isInManHuntMode();
    static bool isValidDamageScenario(PuppetInfo* capOwnerInfo, bool& capOwnerIsSeeker, bool& targetIsSeeker);
    static bool isCapOwnerInSafeZone(PuppetInfo* capOwnerInfo);
    static bool isTargetInSafeZone(al::LiveActor* targetActor, bool targetIsSeeker);
    static bool isTankCapture(const char* hackName);
    static void sendDamageToPlayer(al::HitSensor* receiver, al::HitSensor* sender);
    static void sendDamageToHackActor(al::HitSensor* receiver, al::HitSensor* sender, al::LiveActor* hackActor);

public:
    static void handleCapDamage(al::HitSensor* sender, al::HitSensor* receiver, 
                               PuppetInfo* capOwnerInfo, bool isInvincible);
};