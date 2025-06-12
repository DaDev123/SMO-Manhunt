#pragma once

#include "server/gamemode/GameModeInfoBase.hpp"
#include "server/gamemode/GameModeTimer.hpp"

struct ManHuntInfo : GameModeInfoBase {
    ManHuntInfo() {
        mMode = GameMode::MANHUNT;
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

    inline bool isPlayerHunting() const { return  mIsPlayerIt; }
    inline bool isPlayerRunning()  const { return !mIsPlayerIt; }
};
