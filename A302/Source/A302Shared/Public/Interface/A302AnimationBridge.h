#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "A302AnimationBridge.generated.h"

UINTERFACE(MinimalAPI)
class UA302AnimationBridge : public UInterface
{
	GENERATED_BODY()
};

class A302SHARED_API IA302AnimationBridge
{
	GENERATED_BODY()

public:
	virtual void PlayAttackAnimation() = 0;
	virtual void PlayBlockAnimation() = 0;
	virtual void PlayInteractAnimation() = 0;
	virtual void PlayDeathAnimation() = 0;
	virtual void PlayTimeKnifeAnimation() = 0;

	virtual void PlayStatueInteractAnimation() = 0;
	virtual void StopStatueInteractAnimation() = 0;
};
