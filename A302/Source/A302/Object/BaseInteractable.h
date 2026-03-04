#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/InteractableInterface.h"
#include "BaseInteractable.generated.h"

class UItemDefinition;

UCLASS()
class A302_API ABaseInteractable : public AActor, public IInteractableInterface
{
	GENERATED_BODY()
		
public:	
	ABaseInteractable();

	UItemDefinition* GetItemDefinition() const { return ItemDefinition; }

protected:
	// 에디터에서 조회 및 수정 가능하도록 UPROPERTY 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	EInteractType CurrentInteractType;

	// 상호작용 시 실행될 로직 (인터페이스 함수 오버라이드)
	virtual void Interact(class AMyCharacter* PlayerCharacter) override;
    
	// 조준 시 띄울 텍스트 (인터페이스 함수 오버라이드)
	virtual FString GetInteractText() override;

	// 현재 할당된 상호작용 타입(Enum)을 반환하는 함수 오버라이드
	virtual EInteractType GetInteractType() override;

	// 시각적 확인을 위한 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// 월드에 놓인 이 액터가 표현하는 아이템 정의
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UItemDefinition> ItemDefinition;
};
