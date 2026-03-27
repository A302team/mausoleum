// Fill out your copyright notice in the Description page of Project Settings.

#include "Object/EscapeRouteBlocker.h"

#include "Components/BoxComponent.h"

AEscapeRouteBlocker::AEscapeRouteBlocker()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	DefaultRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultRoot"));
	RootComponent = DefaultRoot;

	BlockerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockerBox"));
	BlockerBox->SetupAttachment(RootComponent);
	BlockerBox->SetCollisionProfileName(TEXT("BlockAll"));
	BlockerBox->SetBoxExtent(FVector(200.0f, 200.0f, 200.0f));
}

void AEscapeRouteBlocker::BeginPlay()
{
	Super::BeginPlay();
	InitialScale = GetActorScale3D();
}

void AEscapeRouteBlocker::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsEscapeOpened)
	{
		return;
	}

	CurrentFadeTime += DeltaTime;
	const float Alpha = FMath::Clamp(1.0f - (CurrentFadeTime / FadeDuration), 0.0f, 1.0f);
	SetActorScale3D(InitialScale * Alpha);

	if (Alpha <= 0.0f && HasAuthority())
	{
		Destroy();
	}
}

void AEscapeRouteBlocker::OpenEscapeRoute()
{
	if (bIsEscapeOpened)
	{
		return;
	}

	bIsEscapeOpened = true;

	// 통과 가능하도록 모든 충돌을 비활성화한다.
	if (HasAuthority())
	{
		TArray<UPrimitiveComponent*> PrimitiveComponents;
		GetComponents<UPrimitiveComponent>(PrimitiveComponents);
		for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
		{
			if (PrimitiveComponent)
			{
				PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
		}
	}
}
