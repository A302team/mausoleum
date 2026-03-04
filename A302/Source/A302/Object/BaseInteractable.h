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
	// 상호작용 시 실행될 로직 (인터페이스 함수 오버라이드)
	virtual void Interact(AActor* Interactor) override;
    
	// 조준 시 띄울 텍스트 (인터페이스 함수 오버라이드)
	virtual FString GetInteractText() const override;

	// 시각적 확인을 위한 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// 월드에 놓인 이 액터가 표현하는 아이템 정의
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UItemDefinition> ItemDefinition;
};
