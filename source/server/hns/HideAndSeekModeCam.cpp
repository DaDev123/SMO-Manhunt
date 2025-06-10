
#include "server/hns/HideAndSeekMode.hpp"
#include "server/Client.hpp"

#include "cameras/CameraPoserActorSpectate.h"

void HideAndSeekMode::createCustomCameraTicket(al::CameraDirector* director) {
    mTicket = director->createCameraFromFactory("CameraPoserActorSpectate", nullptr, 0, 5, sead::Matrix34f::ident);
}

void HideAndSeekMode::updateSpectateCam(PlayerActorBase* playerBase) {
    // If the spectate camera ticket is active, get the camera poser
    al::CameraPoser*    curPoser = nullptr;
    al::CameraDirector* director = mCurScene->getCameraDirector();

    if (director) {
        al::CameraPoseUpdater* updater = director->getPoseUpdater(0);
        if (updater && updater->mTicket) {
            curPoser = updater->mTicket->mPoser;
        }
    }

    // Verify 100% that this poser is the actor spectator
    if (curPoser && al::isEqualString(curPoser->getName(), "CameraPoserActorSpectate")) {
        cc::CameraPoserActorSpectate* spectatePoser = (cc::CameraPoserActorSpectate*)curPoser;
        spectatePoser->setPlayer(playerBase);

        // Increase or decrease spectate index using D-pad only
        int indexDirection = 0;
        if (al::isPadTriggerRight(-1)) { indexDirection =  1; } // Right: Move index right
        if (al::isPadTriggerLeft(-1))  { indexDirection = -1; } // Left: Move index left

        // Create list of valid spectate targets (other players in same stage)
        sead::SafeArray<PuppetInfo*, 0x10> spectateTargets;
        int spectateCount = 0; // Track actual number of elements
        
        for (int i = 0; i < mPuppetHolder->getSize(); i++) {
            PuppetInfo* other = Client::getPuppetInfo(i);
            
            if (!other || !other->isConnected || !other->isInSameStage) {
                continue;
            }
            
            // Only spectate players in the same game mode or legacy mode
            if (other->gameMode != mMode && other->gameMode != GameMode::LEGACY) {
                continue;
            }
            
            // Add to array if we have space
            if (spectateCount < spectateTargets.size()) {
                spectateTargets[spectateCount] = other;
                spectateCount++;
            }
        }

        s32 size = spectateCount;

        // Force index to decrease if our current index got out of bounds
        if (size <= mSpectateIndex) {
            mSpectateIndex = size;
            indexDirection = -1; // Move index left
        }

        PuppetInfo* other = (
            0 <= mSpectateIndex && mSpectateIndex < size
            ? spectateTargets[mSpectateIndex]
            : nullptr
        );

        // Force index to decrease if our current target changes to another stage or disconnects
        if (indexDirection == 0 && other && (!other->isInSameStage || !other->isConnected)) {
            indexDirection = -1; // Move index left
        }

        if (indexDirection != 0) {
            // Loop over indices until we find a valid player or return to self (-1)
            do {
                mSpectateIndex += indexDirection;

                // Circular loop the index around
                if (mSpectateIndex < -1) {
                    mSpectateIndex = size - 1;
                } else if (size <= mSpectateIndex) {
                    mSpectateIndex = -1;
                }

                other = (0 <= mSpectateIndex ? spectateTargets[mSpectateIndex] : nullptr);
            } while (other && (!other->isInSameStage || !other->isConnected));
        }

        // If no index and no size change is happening, end here
        if (mPrevSpectateIndex == mSpectateIndex && mPrevSpectateCount == size) {
            return;
        }

        // Apply index to target actor and HUD
        if (other) {
            spectatePoser->setTargetActor(&other->playerPos);
        } else {
            spectatePoser->setTargetActor(al::getTransPtr(playerBase));
        }

        mPrevSpectateIndex = mSpectateIndex;
        mPrevSpectateCount = size;
    }
}