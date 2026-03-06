#include "Object/BaseInteractable.h"
#include "Math/UnrealMathUtility.h"
#include "Character/MyCharacter.h"
#include "GameData/ItemDefinition.h"

ABaseInteractable::ABaseInteractable()
{
	PrimaryActorTick.bCanEverTick = false;
	
	CurrentInteractType = EInteractType::Hold;
	
	// 스태틱 메시 컴포넌트 생성 및 루트 설정
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	RootComponent = Mesh;
	
	int32 RandomIndex = FMath::RandRange(0, static_cast<int32>(EInteractType::MAX) - 1);
	CurrentInteractType = static_cast<EInteractType>(RandomIndex);
}

void ABaseInteractable::Interact(AMyCharacter* PlayerCharacter)
{
	// 상호작용한 캐릭터를 로그로 출력
	if (PlayerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Interaction] %s interacted with %s!"), *PlayerCharacter->GetName(), *GetName());
	}
}

FString ABaseInteractable::GetInteractText()
{
	if (ItemDefinition && !ItemDefinition->DisplayName.IsEmpty())
	{
		return FString::Printf(TEXT("%s (Interact)"), *ItemDefinition->DisplayName.ToString());
	}

	return FString::Printf(TEXT("%s (Interact)"), *GetName());
}

EInteractType ABaseInteractable::GetInteractType()
{
	return CurrentInteractType;
}