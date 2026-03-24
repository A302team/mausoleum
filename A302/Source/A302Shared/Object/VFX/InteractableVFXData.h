#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetUserData.h"
#include "InteractableVFXData.generated.h"

class UNiagaraSystem;

/**
 * StaticMesh 에셋에 직접 저장되는 VFX 정보.
 * 
 * 이 클래스를 사용하면 메시 자체에 VFX 데이터를 저장할 수 있습니다.
 * 매핑 테이블 없이 메시만 바꾸면 자동으로 올바른 VFX가 적용됩니다.
 * 
 * 사용법:
 * 1. StaticMesh 에셋을 에디터에서 열기
 * 2. Details 패널 → Asset User Data 섹션
 * 3. [+] Add Asset User Data → InteractableVFXData 선택
 * 4. Disappear VFX 필드에 Niagara System 할당
 * 5. Save Asset
 * 
 * 이후 해당 메시를 사용하는 모든 Interactable이 자동으로 이 VFX를 재생합니다.
 */
UCLASS(BlueprintType)
class A302SHARED_API UInteractableVFXData : public UAssetUserData
{
	GENERATED_BODY()

public:
	/**
	 * 이 메시를 사용하는 Interactable이 사라질 때 재생할 VFX.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	TObjectPtr<UNiagaraSystem> DisappearVFX = nullptr;

	/**
	 * VFX 스케일 배율 (선택적).
	 * 0 이하면 기본값 1.0 사용.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX", meta = (ClampMin = "0.0"))
	float VFXScale = 0.0f;
};
