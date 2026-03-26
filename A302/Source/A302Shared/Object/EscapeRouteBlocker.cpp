// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/EscapeRouteBlocker.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"

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

	// 클라이언트/서버 모두 GameState가 초기화될 때까지 대기하다가 바인딩합니다.
	if (!bGameStateBound)
	{
		if (AA302GameState* GameState = Cast<AA302GameState>(GetWorld()->GetGameState()))
		{
			GameState->OnGamePhaseChanged().AddUObject(this, &AEscapeRouteBlocker::OnGamePhaseChanged);
			bGameStateBound = true;

			// 이미 미션이 끝난 상태러 늦게 들어온 경우 바로 열어줍니다.
			if (GameState->GamePhase == EGamePhase::Ended)
			{
				OpenEscapeRoute();
			}
		}
	}

	if (bIsEscapeOpened)
	{
		CurrentFadeTime += DeltaTime;
		
		float Alpha = FMath::Clamp(1.0f - (CurrentFadeTime / FadeDuration), 0.0f, 1.0f);
		
		// 서서히 작아지는 연출 (포그 등도 작아지면서 사라짐)
		SetActorScale3D(InitialScale * Alpha);

		if (Alpha <= 0.0f)
		{
			// 복제된 액터는 서버에서만 Destroy해야 클라이언트에도 자동으로 제거됨
			if (HasAuthority())
			{
				Destroy();
			}
		}
	}
}

void AEscapeRouteBlocker::OnGamePhaseChanged(EGamePhase PreviousPhase, EGamePhase NewPhase, float ServerTime)
{
	if (NewPhase == EGamePhase::Ended)
	{
		OpenEscapeRoute();
	}
}

void AEscapeRouteBlocker::OpenEscapeRoute()
{
	if (bIsEscapeOpened) return;
	bIsEscapeOpened = true;

	// 통과할 수 있도록 모든 충돌을 꺼줍니다
	if (HasAuthority())
	{
		TArray<UPrimitiveComponent*> Prims;
		GetComponents<UPrimitiveComponent>(Prims);
		for (UPrimitiveComponent* Prim : Prims)
		{
			Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}


