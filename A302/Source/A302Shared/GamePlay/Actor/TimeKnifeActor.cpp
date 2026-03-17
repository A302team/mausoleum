#include "GamePlay/Actor/TimeKnifeActor.h"

ATimeKnifeActor::ATimeKnifeActor()
{
    PrimaryActorTick.bCanEverTick = false;
    SetActorHiddenInGame(true);
}
