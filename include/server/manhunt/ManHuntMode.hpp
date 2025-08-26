#pragma once

#include <math.h>
#include "al/camera/CameraTicket.h"
#include "server/gamemode/GameModeBase.hpp"
#include "server/gamemode/GameModeInfoBase.hpp"
#include "server/gamemode/GameModeConfigMenu.hpp"
#include "server/gamemode/GameModeTimer.hpp"
#include "server/manhunt/ManHuntConfigMenu.hpp"

// Forward declarations
namespace al {
    class HitSensor;
    class LiveActor;
}
class PuppetInfo;

struct ManHuntInfo : GameModeInfoBase {
    ManHuntInfo() { mMode = GameMode::MANHUNT; }
    bool mIsPlayerHunting = false;
    bool mIsUseGravity = false;
    bool mIsUseGravityCam = false;
    GameTime mHidingTime;
};

class ManHuntMode : public GameModeBase {
    public:
        ManHuntMode(const char* name);

        void init(GameModeInitInfo const& info) override;

        virtual void begin() override;
        virtual void update() override;
        virtual void end() override;

        bool isUseNormalUI() const override { return true; }

        bool isPlayerHunting() const { return mInfo->mIsPlayerHunting; };

        void setPlayerTagState(bool state) { mInfo->mIsPlayerHunting = state; }

        void enableGravityMode() {mInfo->mIsUseGravity = true;}
        void disableGravityMode() { mInfo->mIsUseGravity = false; }
        bool isUseGravity() const { return mInfo->mIsUseGravity; }

        void setCameraTicket(al::CameraTicket *ticket) {mTicket = ticket;}

        bool isPlayerNearOdysseyBarrier(al::LiveActor* player);

        bool isPlayerInSafeZone(al::LiveActor* player);

        StageScene* getCurrentScene() const { return mCurScene; }
        PuppetHolder* getPuppetHolder() const { return mPuppetHolder; }

        // Cap damage handling
        bool handleCapAttack(al::HitSensor* sender, al::HitSensor* receiver, PuppetInfo* attackerInfo);

    private:
        float mInvulnTime = 0.0f;
        GameModeTimer* mModeTimer = nullptr;
        ManHuntIcon *mModeLayout = nullptr;
        ManHuntInfo* mInfo = nullptr;
        al::CameraTicket *mTicket = nullptr;

        // Helper methods for cap damage
        bool isPlayerHuntingByInfo(PuppetInfo* puppetInfo) const;
        bool findReceiverPlayerInfo(al::HitSensor* receiver, al::LiveActor*& receiverActor, bool& isHunting, bool& isInHack, const char*& hackName);
        bool checkSafeZone(const sead::Vector3f& attackerPos, al::LiveActor* receiverActor, bool attackerIsHunting, bool receiverIsHunting);
        void performAttack(al::HitSensor* sender, al::HitSensor* receiver, al::LiveActor* receiverActor, bool receiverIsInHack);


        bool mIsCapInvincible = false;
        float mCapInvulnTimer = 0.0f;


};