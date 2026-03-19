// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/SpawnArea.h"
#include "Components/ShapeComponent.h"
#include "Kismet/KismetMathLibrary.h"

ASpawnArea::ASpawnArea()
{
    // 기본적으로 Tick 비활성화 (성능 최적화)
    PrimaryActorTick.bCanEverTick = false;
}

FVector ASpawnArea::GetRandomPointInBox() const
{
    if (UShapeComponent* ShapeComp = GetCollisionComponent())
    {
        // 트리거 박스의 중심점과 범위를 가져와 그 안의 무작위 좌표를 생성
        FVector Origin = ShapeComp->Bounds.Origin;
        FVector BoxExtent = ShapeComp->Bounds.BoxExtent;
        
        return UKismetMathLibrary::RandomPointInBoundingBox(Origin, BoxExtent);
    }
    
    return GetActorLocation();
}
