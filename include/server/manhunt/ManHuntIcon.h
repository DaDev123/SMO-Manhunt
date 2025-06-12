#pragma once

#include "al/layout/LayoutActor.h"
#include "al/layout/LayoutInitInfo.h"
#include "al/util/NerveUtil.h"

#include "server/manhunt/ManHuntInfo.hpp"
#include "layouts/LayoutPlayerList.h"

extern int compassTargetIndex;

// TODO: kill layout if going through loading zone or paused

class ManHuntIcon : public al::LayoutActor, LayoutPlayerList {
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
    protected:
        const char* getRoleIcon(bool isIt) override;
        GameMode getGameMode() override;
        bool isMeIt() override;

    private:
        struct ManHuntInfo* mInfo;
};

namespace {
    NERVE_HEADER(ManHuntIcon, Appear)
    NERVE_HEADER(ManHuntIcon, Wait)
    NERVE_HEADER(ManHuntIcon, End)
}
