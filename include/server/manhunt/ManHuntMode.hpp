#pragma once

#include <math.h>
#include <basis/seadTypes.h>

#include "al/camera/CameraTicket.h"
#include "server/gamemode/GameModeBase.hpp"
#include "server/gamemode/GameModeInfoBase.hpp"
#include "server/gamemode/GameModeConfigMenu.hpp"
#include "server/gamemode/GameModeTimer.hpp"
#include "server/manhunt/ManHuntConfigMenu.hpp"

#include "packets/Packet.h"

struct ManHuntInfo : GameModeInfoBase {
    ManHuntInfo() { mMode = GameMode::MANHUNT; }
    bool mIsPlayerIt = false;
    bool mIsUseGravity = false;
    bool mIsUseGravityCam = false;
    bool mIsUseSlipperyGround = true;
    GameTime mHidingTime;

    inline bool isPlayerHunting() const { return  mIsPlayerIt; }
    inline bool isPlayerRunning()  const { return !mIsPlayerIt; }
};

enum TagUpdateType : u8 {
    TIME                 = 1 << 0,
    STATE                = 1 << 1
};

struct PACKED ManHuntPacket : Packet {
    ManHuntPacket() : Packet() { this->mType = PacketType::GAMEMODEINF; mPacketSize = sizeof(ManHuntPacket) - sizeof(Packet);};
    TagUpdateType updateType;
    bool1 isIt = false;
    u8 seconds;
    u16 minutes;
};

class ManHuntMode : public GameModeBase {
    public:
        ManHuntMode(const char* name);

        void init(GameModeInitInfo const& info) override;

        void begin() override;
        void update() override;
        void end() override;
    
        void pause() override;
        void unpause() override;

        bool isUseNormalUI() const override { return true; }

        void processPacket(Packet* packet) override;
        Packet* createPacket() override;

        inline bool isPlayerHunting() const { return mInfo->isPlayerHunting(); }
        inline bool isPlayerRunning()  const { return mInfo->isPlayerRunning();  }

        float getInvulnTime() const { return mInvulnTime; }

        void setPlayerTagState(bool state) { mInfo->mIsPlayerIt = state; }

        void enableGravityMode() {mInfo->mIsUseGravity = true;}
        void disableGravityMode() { mInfo->mIsUseGravity = false; }
        bool isUseGravity() const { return mInfo->mIsUseGravity; }

        void setCameraTicket(al::CameraTicket* ticket) { mTicket = ticket; }

        bool isPlayerNearOdysseyBarrier(al::LiveActor* player);

        bool isPlayerInSafeZone(al::LiveActor* player);

    private:
        float mInvulnTime = 0.0f;
        GameModeTimer* mModeTimer = nullptr;
        ManHuntIcon *mModeLayout = nullptr;
        ManHuntInfo* mInfo = nullptr;
        al::CameraTicket *mTicket = nullptr;

        void updateTagState(bool isSeeking);
};