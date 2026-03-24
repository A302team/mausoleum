#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractableInterface.generated.h"

UENUM(BlueprintType)
enum class EInteractType : uint8
{
	Hold UMETA(DisplayName = "Hold (꾹 누르기)"),
	QTE  UMETA(DisplayName = "QTE (무작위 키 입력)"),
	MAX  UMETA(Hidden)
};

UINTERFACE(MinimalAPI)
class UInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

class A302SHARED_API IInteractableInterface
{
	GENERATED_BODY()

public:
	// UI에 띄울 텍스트 반환 (예: "서랍장 열기", "조사하기")
	virtual FString GetInteractText() = 0;

	// 이 오브젝트의 상호작용 타입 반환
	virtual EInteractType GetInteractType() = 0;

	// 상호작용이 "최종적으로 성공"했을 때 실행될 로직 (문 열림, 파괴 등)
	// 누가 상호작용을 시도했는지 알기 위해 플레이어 포인터를 넘겨줍니다.
	virtual void Interact(class ACharacter* PlayerCharacter) = 0;

	// 지속적인 상호작용(Hold) 진행도를 서버사이드에서 처리하도록 전달
	virtual void OnServerHoldProgress(float DeltaTime, class ACharacter* Interactor) {}
};
