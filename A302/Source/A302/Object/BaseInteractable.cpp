#include "Object/BaseInteractable.h"

ABaseInteractable::ABaseInteractable()
{
	PrimaryActorTick.bCanEverTick = false;

	// 스태틱 메시 컴포넌트 생성 및 루트 설정
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	RootComponent = Mesh;
}

void ABaseInteractable::Interact(AActor* Interactor)
{
	// 상호작용한 캐릭터를 로그로 출력
	if (Interactor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Interaction] %s interacted with %s!"), *Interactor->GetName(), *GetName());
	}
}

FString ABaseInteractable::GetInteractText() const
{
	// 나중에 UI에 띄울 텍스트
	return FString::Printf(TEXT("%s (Interact)"), *GetName());
}