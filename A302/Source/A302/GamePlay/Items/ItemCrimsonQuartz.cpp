#include "GamePlay/Items/ItemCrimsonQuartz.h"

#include "Character/Components/MaliceComponent.h"
#include "Character/MyCharacter.h"
#include "GameData/Items/ItemInstance.h"

FString UItemCrimsonQuartz::BuildInspectionMessage(int32 RawMaliceCount)
{
	if (RawMaliceCount <= 0)
	{
		return TEXT("아무런 반응도 일어나지 않고 수정구가 사라졌습니다.");
	}

	if (RawMaliceCount <= 2)
	{
		return TEXT("수정구가 붉은 빛을 약하게 뿜어내다가 사라졌습니다.");
	}

	return TEXT("수정구가 강한 붉은 빛을 뿜어내다가 사라졌습니다. 저 자는 악인입니다.");
}

bool UItemCrimsonQuartz::CanUse_Implementation(ACharacter* Instigator, const FItemTargetData& TargetData) const
{
	const AMyCharacter* OwnerCharacter = Cast<AMyCharacter>(Instigator);
	const AMyCharacter* TargetCharacter = Cast<AMyCharacter>(TargetData.TargetActor);

	if (!OwnerCharacter || !TargetCharacter)
	{
		return false;
	}

	return OwnerCharacter != TargetCharacter;
}

bool UItemCrimsonQuartz::Use_Implementation(ACharacter* Instigator, const FItemTargetData& TargetData)
{
	if (!CanUse_Implementation(Instigator, TargetData))
	{
		return false;
	}

	AMyCharacter* OwnerCharacter = Cast<AMyCharacter>(Instigator);
	AMyCharacter* TargetCharacter = Cast<AMyCharacter>(TargetData.TargetActor);
	if (!OwnerCharacter || !TargetCharacter)
	{
		return false;
	}

	const UItemDefinition* ItemDefinition = GetDefinition();
	if (!ItemDefinition)
	{
		return false;
	}

	OwnerCharacter->Server_RequestTargetedItemUse(const_cast<UItemDefinition*>(ItemDefinition), TargetCharacter);

	if (UItemInstance* Inst = GetInstance())
	{
		Inst->Consume(1);
	}

	OnItemUsed(OwnerCharacter);
	return true;
}

bool UItemCrimsonQuartz::ResolveServerTargetedUse(
	AMyCharacter* OwnerCharacter,
	AActor* TargetActor,
	FString& OutSystemMessage
) const
{
	const AMyCharacter* TargetCharacter = Cast<AMyCharacter>(TargetActor);
	if (!OwnerCharacter || !TargetCharacter || TargetCharacter == OwnerCharacter)
	{
		return false;
	}

	const UMaliceComponent* TargetMaliceComponent = TargetCharacter->FindComponentByClass<UMaliceComponent>();
	const int32 RawMaliceCount = TargetMaliceComponent ? TargetMaliceComponent->GetRawMalice() : 0;

	OutSystemMessage = BuildInspectionMessage(RawMaliceCount);
	return true;
}
