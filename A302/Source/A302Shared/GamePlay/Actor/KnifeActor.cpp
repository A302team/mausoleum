#include "GamePlay/Actor/KnifeActor.h"

AKnifeActor::AKnifeActor()
{
    PrimaryActorTick.bCanEverTick = false;
    SetActorHiddenInGame(true);
}
