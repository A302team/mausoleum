#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractableInterface.generated.h"

UINTERFACE(MinimalAPI)
class UInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

class A302_API IInteractableInterface
{
	GENERATED_BODY()

public:
	/** 상호작용 시 호출될 함수 */
	virtual void Interact(AActor* Interactor) = 0;
    
	/** (선택) 조준했을 때 화면에 띄울 텍스트 */
	virtual FString GetInteractText() const { return TEXT("Object"); }
};