#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "A302TimedKillEventBridge.generated.h"

UINTERFACE(MinimalAPI)
class UA302TimedKillEventBridge : public UInterface
{
	GENERATED_BODY()
};

class A302SHARED_API IA302TimedKillEventBridge
{
	GENERATED_BODY()

public:
	virtual void NotifyTimedKillConfirmed() = 0;
	virtual void CancelTimedKillCountdown() = 0;
};
