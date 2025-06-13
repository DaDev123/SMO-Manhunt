#include "server/manhunt/ManHuntConfigMenu.hpp"

#include "server/manhunt/ManHuntInfo.hpp"

ManHuntConfigMenu::ManHuntConfigMenu() : GameModeConfigMenu() {
    mItems = new sead::SafeArray<sead::WFixedSafeString<0x200>, mItemCount>();
    mItems->mBuffer[0].copy(u"Toggle ManHunt Gravity (OFF)");
    mItems->mBuffer[1].copy(u"Mario Collision (ON)    ");
    mItems->mBuffer[2].copy(u"Mario Bounce (ON)       ");
    mItems->mBuffer[3].copy(u"Cappy Collision (ON)   ");
    mItems->mBuffer[4].copy(u"Cappy Bounce (ON)      ");
}

const sead::WFixedSafeString<0x200>* ManHuntConfigMenu::getStringData() {
    // Gravity
    const char16_t* gravity = (
        ManHuntInfo::mIsUseGravity
        ? u"Toggle H&S Gravity (ON) "
        : u"Toggle H&S Gravity (OFF)"
    );

    // Collision Toggles
    const char16_t* marioCollision = (
        ManHuntInfo::mHasMarioCollision
        ? u"Mario Collision (ON)    "
        : u"Mario Collision (OFF)   "
    );
    const char16_t* marioBounce = (
        ManHuntInfo::mHasMarioBounce
        ? u"Mario Bounce (ON)       "
        : u"Mario Bounce (OFF)      "
    );
    const char16_t* cappyCollision = (
        ManHuntInfo::mHasCappyCollision
        ? u"Cappy Collision (ON)    "
        : u"Cappy Collision (OFF)   "
    );
    const char16_t* cappyBounce = (
        ManHuntInfo::mHasCappyBounce
        ? u"Cappy Bounce (ON)       "
        : u"Cappy Bounce (OFF)      "
    );

    mItems->mBuffer[0].copy(gravity);
    mItems->mBuffer[1].copy(marioCollision);
    mItems->mBuffer[2].copy(marioBounce);
    mItems->mBuffer[3].copy(cappyCollision);
    mItems->mBuffer[4].copy(cappyBounce);

    return mItems->mBuffer;
}

GameModeConfigMenu::UpdateAction ManHuntConfigMenu::updateMenu(int selectIndex) {
    switch (selectIndex) {
        case 0: {
            ManHuntInfo::mIsUseGravity = !ManHuntInfo::mIsUseGravity;
            return UpdateAction::REFRESH;
        }
        case 1: {
            ManHuntInfo::mHasMarioCollision = !ManHuntInfo::mHasMarioCollision;
            return UpdateAction::REFRESH;
        }
        case 2: {
            ManHuntInfo::mHasMarioBounce = !ManHuntInfo::mHasMarioBounce;
            return UpdateAction::REFRESH;
        }
        case 3: {
            ManHuntInfo::mHasCappyCollision = !ManHuntInfo::mHasCappyCollision;
            return UpdateAction::REFRESH;
        }
        case 4: {
            ManHuntInfo::mHasCappyBounce = !ManHuntInfo::mHasCappyBounce;
            return UpdateAction::REFRESH;
        }
        default: {
            return UpdateAction::NOOP;
        }
    }
}
