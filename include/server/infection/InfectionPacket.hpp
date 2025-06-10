#pragma once

#include "packets/GameModeInf.h"

struct InfectionUpdateTypes {
    enum Type : u8 {
        TIME  = 1 << 0,
        STATE = 1 << 1
    };
};
typedef InfectionUpdateTypes::Type InfectionUpdateType;

struct PACKED InfectionPacket : GameModeInf<InfectionUpdateType> {
    InfectionPacket() : GameModeInf() {
        setGameMode(GameMode::INFECTION);
        mPacketSize = sizeof(InfectionPacket) - sizeof(Packet);
    };
    bool1 isIt = false;
    u8    seconds;
    u16   minutes;
};
