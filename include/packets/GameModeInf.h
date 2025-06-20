#pragma once

#include "Packet.h"
#include "server/Client.hpp"

template <typename UpdateType>
struct PACKED GameModeInf : Packet {
    GameModeInf() : Packet() {
        this->mType = PacketType::GAMEMODEINF;
        mPacketSize = sizeof(GameModeInf) - sizeof(Packet);
    };
    u8 mModeAndType = 0;

    GameMode gameMode() {
        GameMode mode = (GameMode)((((mModeAndType >> 4) + 1) % 16) - 1);

        if (mode == GameMode::LEGACY) {
            u8 type = mModeAndType & 0x0f;

            // STATE ManHunt
            if (type == 3) {
                return GameMode::LEGACY;
            }

            
        }

        return mode;
    }

    UpdateType updateType() {
        return static_cast<UpdateType>(mModeAndType & 0x0f);
    }

    void setGameMode(GameMode mode) {
        mModeAndType = (mode << 4) | (mModeAndType & 0x0f);
    }

    void setUpdateType(UpdateType type) {
        mModeAndType = (mModeAndType & 0xf0) | (type & 0x0f);
    }
};

struct PACKED DisabledGameModeInf : GameModeInf<u8> {
    DisabledGameModeInf(nn::account::Uid userID) : GameModeInf() {
        setGameMode(GameMode::NONE);
        setUpdateType(3); // so that legacy Hide&Seek and Sardines clients will parse isIt = false
        mUserID     = userID;
        mPacketSize = sizeof(DisabledGameModeInf) - sizeof(Packet);
    };
    bool isIt    = false;
    u8   seconds = 0;
    u16  minutes = 0;
};
