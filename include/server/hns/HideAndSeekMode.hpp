#pragma once

#include "al/camera/CameraTicket.h"
#include "sead/container/seadSafeArray.h"

#include "server/gamemode/GameModeBase.hpp"
#include "server/gamemode/GameModeTimer.hpp"
#include "server/hns/HideAndSeekIcon.h"
#include "server/hns/HideAndSeekInfo.hpp"

class HideAndSeekMode : public GameModeBase {
    public:
        HideAndSeekMode(const char* name);

        void init(GameModeInitInfo const& info) override;

        void begin() override;
        void update() override;
        void end() override;

        void pause() override;
        void unpause() override;

        bool showNameTag(PuppetInfo* other) override;

        bool checkPuppetCapCollision(PlayerActorHakoniwa* playerBase);

        void debugMenuControls(sead::TextWriter* gTextWriter) override;

        bool isUseNormalUI() const override { return true; }

        void processPacket(Packet* packet) override;
        Packet* createPacket() override;

        inline bool isPlayerSeeking() const { return mInfo->isPlayerSeeking(); }
        inline bool isPlayerHiding()  const { return mInfo->isPlayerHiding();  }

        float getInvulnTime() const { return mInvulnTime; }

        void enableGravityMode() { mInfo->mIsUseGravity = true; }
        void disableGravityMode() { mInfo->mIsUseGravity = false; }
        bool isUseGravity() const { return mInfo->mIsUseGravity; }
        void onBorderPullBackFirstStep(al::LiveActor* actor) override;

        bool hasCustomCamera() const override { return true; }
        void createCustomCameraTicket(al::CameraDirector* director) override;
        void updateSpectateCam(PlayerActorBase* playerBase); // Updates the spectator camera

        bool hasMarioCollision() override { return HideAndSeekInfo::mHasMarioCollision; }
        bool hasMarioBounce()    override { return HideAndSeekInfo::mHasMarioBounce;    }
        bool hasCappyCollision() override { return HideAndSeekInfo::mHasCappyCollision; }
        bool hasCappyBounce()    override { return HideAndSeekInfo::mHasCappyBounce;    }

        bool isPlayerNearOdysseyBarrier(al::LiveActor* player);

    private:
        float             mInvulnTime = 0.0f;
        GameModeTimer*    mModeTimer  = nullptr;
        HideAndSeekIcon*  mModeLayout = nullptr;
        HideAndSeekInfo*  mInfo       = nullptr;
        al::CameraTicket* mTicket     = nullptr;

        // Spectate camera variables
        int mPrevSpectateCount = 0;
        int mPrevSpectateIndex = -2;
        int mSpectateIndex     = -1; // -1 means spectating self

        void updateTagState(bool isSeeking);
};