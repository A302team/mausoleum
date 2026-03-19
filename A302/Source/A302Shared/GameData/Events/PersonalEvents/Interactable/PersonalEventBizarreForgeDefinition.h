#pragma once

#include "CoreMinimal.h"
#include "GameData/Events/PersonalEvents/PersonalEventDefinition.h"
#include "PersonalEventBizarreForgeDefinition.generated.h"

class UItemDefinition;

UCLASS(BlueprintType)
class A302SHARED_API UPersonalEventBizarreForgeDefinition : public UPersonalEventDefinition
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BizarreForge")
	TObjectPtr<UItemDefinition> MaliciousSwordDefinition = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BizarreForge")
	TObjectPtr<UItemDefinition> MaliciousShieldDefinition = nullptr;
};
