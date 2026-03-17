#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "A302CharacterBridge.generated.h"

UINTERFACE(MinimalAPI)
class UA302CharacterBridge : public UInterface
{
	GENERATED_BODY()
};

class A302SHARED_API IA302CharacterBridge
{
	GENERATED_BODY()

public:
	virtual void NotifyKilledCharacter() = 0;
	virtual void NotifyTimedKnifeAttackSucceeded() = 0;
	virtual void RegisterTimedKillEvent(UObject* EventInstance) = 0;
	virtual void ClearTimedKillEvent(UObject* EventInstance) = 0;
	virtual void ForceDeathByPersonalEvent() = 0;
	virtual void SetTimedKnifeAttackInProgress(bool bInProgress) = 0;
	virtual void EquipKnifeWeapon() = 0;
	virtual void EquipTimeKnifeWeapon() = 0;
	virtual void EquipShieldWeapon() = 0;
	virtual void ShowWeapon() = 0;
	virtual void HideWeapon() = 0;
	virtual void PlayShieldBlockPresentation() = 0;
	virtual void SetQTEInputMode(bool bIsQTE) = 0;
	virtual void BroadcastPublicMaliceAnnouncement(const FString& PlayerName, int32 MaliceCount) = 0;
};
