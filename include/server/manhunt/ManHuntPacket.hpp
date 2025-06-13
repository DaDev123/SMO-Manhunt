#pragma once

#include "packets/GameModeInf.h"

struct ManHuntUpdateTypes {
    enum Type : u8 {
        TIME  = 1 << 0,
        STATE = 1 << 1
    };
};
typedef ManHuntUpdateTypes::Type ManHuntUpdateType;

struct PACKED ManHuntPacket : GameModeInf<ManHuntUpdateType> {
    ManHuntPacket() : GameModeInf() {
        setGameMode(GameMode::MANHUNT);
        mPacketSize = sizeof(ManHuntPacket) - sizeof(Packet);
    };
    bool1 isIt = false;
    u8    seconds;
    u16   minutes;
};
