#pragma once

#include "types.h"

#include "sead/math/seadVector.h"
#include "sead/math/seadQuat.hpp"

#include "al/util.hpp"

#include "puppets/PuppetInfo.h"

bool isPartOf(const char* w1, const char* w2);

int indexOf(char* w1, char c1);

void logVector(const char* vectorName, sead::Vector3f vector);

void logQuat(const char* quatName, sead::Quatf quat);

sead::Vector3f QuatToEuler(sead::Quatf* quat);

float vecMagnitude(sead::Vector3f const& input);
float vecDistance(sead::Vector3f const& a, sead::Vector3f const& b);
float vecDistanceSq(sead::Vector3f const& a, sead::Vector3f const& b);

float quatAngle(sead::Quatf const& q1, sead::Quatf& q2);

bool isInCostumeList(const char* costumeName);

const char* tryGetPuppetCapName(PuppetInfo* info);
const char* tryGetPuppetBodyName(PuppetInfo* info);

const char* tryConvertName(const char* className);

void killMainPlayer(al::LiveActor* actor);
void killMainPlayer(PlayerActorHakoniwa* mainPlayer);

__attribute__((used)) static const char* costumeNames[] = {
    "Mario",
    "Mario64",
    "Mario64Metal",
    "MarioAloha",
    "MarioArmor",
    "MarioBone",
    "MarioCaptain",
    "MarioClown",
    "MarioColorClassic",
    "MarioColorGold",
    "MarioColorLuigi",
    "MarioColorWaluigi",
    "MarioColorWario",
    "MarioCook",
    "MarioDiddyKong",
    "MarioDoctor",
    "MarioExplorer",
    "MarioFootball",
    "MarioGolf",
    "MarioGunman",
    "MarioHakama",
    "MarioHappi",
    "MarioKing",
    "MarioKoopa",
    "MarioMaker",
    "MarioMechanic",
    "MarioNew3DS",
    "MarioPainter",
    "MarioPeach",
    "MarioPilot",
    "MarioPirate",
    "MarioPoncho",
    "MarioPrimitiveMan",
    "MarioSailor",
    "MarioScientist",
    "MarioShopman",
    "MarioSnowSuit",
    "MarioSpaceSuit",
    "MarioSuit",
    "MarioSwimwear",
    "MarioTailCoat",
    "MarioTuxedo",
    "MarioUnderwear",
    "MarioSanta", // DLC
    "MarioArmorWestern", // DLC
    "MarioBatter", // DLC
    "MarioConductor", // DLC
    "MarioHariet", // DLC
    "MarioRango", // DLC
    "MarioRsv", // DLC
    "MarioRango", // DLC
    "MarioSatellite", // DLC
    "MarioSpewart", // DLC
    "MarioSunshine", // DLC
    "MarioTopper", // DLC
    "MarioZombie", // DLC
    "MarioWooper" // Custom
    "MarioTanooki" // Custom
};

struct HackActorName {
    const char* className;
    const char* hackName;
};

// attribute otherwise the build log is spammed with unused warnings
__attribute__((used)) static HackActorName classHackNames[] = {
    {"SenobiGeneratePoint", "Senobi"},
    {"KuriboPossessed", "Kuribo"},
    {"KillerLauncher", "Killer"},
    {"KillerLauncherMagnum", "KillerMagnum"},
    {"FireBrosPossessed", "FireBros"},
    {"HammerBrosPossessed", "HammerBros"},
    {"ElectricWire", "ElectricWireMover"},
    {"TRexSleep", "TRex"},
    {"TRexPatrol", "TRex"},
    {"WanwanBig", "Wanwan"},  // FIXME: this will make chain chomp captures always be the small
                              // variant for syncing
    {"Koopa","KoopaHack"}
};

struct Transform
{
    sead::Vector3f* position;
    sead::Quatf* rotation;
};

// From Boss Room Unity Example
class VisualUtils
{
public:
    /*
    * @brief Smoothly interpolates towards the parent transform.
    * @param moveTransform The transform to interpolate
    * @param targetTransform The transform to interpolate towards.
    * @param timeDelta Time in seconds that has elapsed, for purposes of interpolation.
    * @param closingSpeed The closing speed in m/s. This is updated by SmoothMove every time it is called, and will drop to 0 whenever the moveTransform has "caught up".
    * @param maxAngularSpeed The max angular speed to to rotate at, in degrees/s.
    */
    static float SmoothMove(
        Transform moveTransform,
        Transform targetTransform,
        float timeDelta,
        float closingSpeed,
        float maxAngularSpeed
    );

    constexpr static const float k_MinSmoothSpeed = 0.1f;
    constexpr static const float k_TargetCatchupTime = 0.2f;
};
