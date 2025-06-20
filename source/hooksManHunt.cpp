#include <cmath>
#include <math.h>

#include "main.hpp"

#include "al/execute/ExecuteOrder.h"
#include "al/execute/ExecuteTable.h"
#include "al/execute/ExecuteTableHolderDraw.h"
#include "al/LiveActor/LiveActor.h"
#include "al/util.hpp"
#include "al/util/AudioUtil.h"
#include "al/util/CameraUtil.h"
#include "al/util/ControllerUtil.h"
#include "al/util/GraphicsUtil.h"
#include "al/util/LiveActorUtil.h"
#include "al/util/NerveUtil.h"
#include "al/util/DemoUtil.h"

#include "game/GameData/GameDataHolderAccessor.h"
#include "game/GameData/GameDataFunction.h"
#include "game/HakoniwaSequence/HakoniwaSequence.h"
#include "game/Player/PlayerActorBase.h"
#include "game/Player/PlayerActorHakoniwa.h"
#include "game/Player/PlayerFunction.h"
#include "game/Player/PlayerHackKeeper.h"
#include "game/StageScene/StageScene.h"

#include "logger.hpp"

#include "sead/container/seadSafeArray.h"
#include "sead/gfx/seadPrimitiveRenderer.h"
#include "sead/heap/seadHeap.h"
#include "sead/math/seadVector.h"

#include "server/gamemode/GameModeBase.hpp"
#include "server/gamemode/GameModeFactory.hpp"
#include "server/gamemode/GameModeManager.hpp"

#include "rs/util.hpp"

void stageSceneHook() {
    StageScene *stageScene;
    __asm("MOV %[result], X0" : [result] "=r"(stageScene));
    isInGame = true;

    auto *pHolder = al::getScenePlayerHolder(stageScene);
    auto *player = (PlayerActorHakoniwa*)al::tryGetPlayerActor(pHolder, 0);
    PlayerActorBase*  playerBase = al::tryGetPlayerActor(pHolder, 0);
    if (!player) return;

    GameDataHolderWriter writer(stageScene->mHolder);
    GameDataFunction::enableCap(writer);
    GameDataFunction::talkCapNearHomeInWaterfall(player);

    if (!GameModeManager::instance()->isModeAndActive(GameMode::MANHUNT)) {
        ShineTowerRocket* odyssey = rs::tryGetShineTowerRocketFromDemoDirector((al::LiveActor*)playerBase);
        if (odyssey) {
            al::tryDeleteEffect((al::LiveActor*)odyssey, "Special1WorldHomeGKBarrier");
        }
    }


}


// ManHunt bool and stuff

namespace al {
    bool trySyncStageSwitchAppearAndKill(LiveActor*);
    const char* getModelName(const LiveActor* actor);
    void startNerveAction(LiveActor*, const char*);
}

void barrierAppearHook(al::LiveActor* thisPtr, const char* actionName) {
    if (!GameModeManager::instance()->isModeAndActive(GameMode::MANHUNT)) {
        return; // Disable functionality if not in MANHUNT mode
    }

    if (al::isEqualString(GameDataFunction::getCurrentStageName(thisPtr), "SkyWorldHomeStage") && 
        al::calcDistanceH(thisPtr, sead::Vector3f{5722.f, 29000.f, -41583.f}) < 200) {
        // thisPtr->kill();
        al::startNerveAction(thisPtr, "Disappear");
    } else {
        al::startNerveAction(thisPtr, actionName);
    }
}


bool compassAlwaysVisible(GameDataHolderAccessor accessor) {
    return GameModeManager::instance()->isModeAndActive(GameMode::MANHUNT);
}
