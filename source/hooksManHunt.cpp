bool manhuntSequenceHook(HakoniwaSequence* sequence) {
    StageScene* stageScene = (StageScene*)sequence->curScene;

    // ManHunt Code

    al::LiveActor* player = nullptr;
    if (pHolder) {
        player = pHolder->getPlayer(0);
    }

    GameDataHolderWriter writer(stageScene->mHolder);
    GameDataFunction::enableCap(writer);
    GameDataFunction::talkCapNearHomeInWaterfall(player);

    if (!GameModeManager::instance()->isModeAndActive(GameMode::HIDEANDSEEK)) {
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
    if (!GameModeManager::instance()->isModeAndActive(GameMode::HIDEANDSEEK)) {
        return; // Disable functionality if not in HIDEANDSEEK mode
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
    return GameModeManager::instance()->isModeAndActive(GameMode::HIDEANDSEEK);
}
