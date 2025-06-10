#pragma once

#include "server/gamemode/GameModeInfoBase.hpp"
#include "server/gamemode/GameModeTimer.hpp"

struct InfectionInfo : GameModeInfoBase {
    InfectionInfo() {
        mMode = GameMode::INFECTION;
    }
    bool     mIsPlayerIt          = false;
    bool     mIsUseGravityCam     = false;
    bool     mIsUseSlipperyGround = true;
    GameTime mHidingTime;

    static bool mIsUseGravity;

    static bool mHasMarioCollision;
    static bool mHasMarioBounce;
    static bool mHasCappyCollision;
    static bool mHasCappyBounce;

    inline bool isPlayerInfected() const { return  mIsPlayerIt; }
    inline bool isPlayerRunning()  const { return !mIsPlayerIt; }
};
