#pragma once

#include "al/layout/LayoutActor.h"
#include "al/layout/LayoutInitInfo.h"
#include "al/util/NerveUtil.h"

#include "logger.hpp"
#include "server/gamemode/GameModeTimer.hpp"

extern int compassTargetIndex;


// TODO: kill layout if going through loading zone or paused

class ManHuntIcon : public al::LayoutActor {
    public:
        ManHuntIcon(const char* name, const al::LayoutInitInfo& initInfo);

        void appear() override;

        bool tryStart();
        bool tryEnd();

        void showHiding();
        void showSeeking();
        
        void exeAppear();
        void exeWait();
        void exeEnd();

        const char* getCurrentCompassTarget();

    private:
        struct ManHuntInfo *mInfo;
};

namespace {
    NERVE_HEADER(ManHuntIcon, Appear)
    NERVE_HEADER(ManHuntIcon, Wait)
    NERVE_HEADER(ManHuntIcon, End)
}