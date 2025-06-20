#pragma once

#include <stdint.h>
#include "algorithms/PlayerAnims.h"
#include "packets/Packet.h"

#include "nn/account.h"

#include "sead/math/seadVector.h"
#include "sead/math/seadQuat.h"

#include "server/gamemode/GameMode.hpp"

struct PuppetInfo {
    // General Puppet Info
    nn::account::Uid playerID;
    char             puppetName[0x10] = {}; // max user account name size is 10 chars, so this could go down to 0xB
    bool             isConnected      = false;
    GameMode         gameMode         = GameMode::NONE;

    // Puppet Translation Info
    sead::Vector3f playerPos = sead::Vector3f(0.f,0.f,0.f);
    sead::Quatf    playerRot = sead::Quatf(0.f,0.f,0.f,0.f);

    // Puppet Stage Info
    u8   scenarioNo      = -1;
    char stageName[0x40] = {};
    bool isInSameStage   = false;

    // Puppet Costume Info
    char costumeBody[0x20] = {};
    char costumeHead[0x20] = {};

    // Puppet Capture Info
    char curHack[0x40]  = {};
    bool isCaptured     = false;
    bool isStartCapture = false;

    // Puppet Model Info
    PlayerAnims::Type curAnim;
    PlayerAnims::Type curSubAnim;
    char              curAnimStr[PACKBUFSIZE]    = {};
    char              curSubAnimStr[PACKBUFSIZE] = {};
    float             blendWeights[6]            = {};
    float             animRate                   = 0.f;
    bool              is2D                       = false;

    // Puppet Hack Cap Info
    sead::Vector3f capPos               = sead::Vector3f(0.f,0.f,0.f);
    sead::Quatf    capRot               = sead::Quatf(0.f,0.f,0.f,0.f);
    char           capAnim[PACKBUFSIZE] = {};
    bool           isCapThrow           = false;
    bool           isHoldThrow          = false;

    // Hide and Seek, Sardines & Infection Gamemode Info
    bool isIt    = false;
    u8   seconds = 0;
    u16  minutes = 0;

    // ManHunt
    inline bool manhuntIsHunting() const { return  isIt; }
    inline bool manhuntIsRunning()  const { return !isIt; }
};
