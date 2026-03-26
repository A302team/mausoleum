#include "Character/Components/Effects/ItemEffectComponent.h"

#include "Character/Components/Inventory/ItemManagerComponent.h"
#include "Character/Components/MaliceComponent.h"
#include "Character/Components/Interaction/CharacterRewardComponent.h"
#include "Character/MyPlayerController.h"
#include "GameData/Items/ItemDefinition.h"
#include "GameData/RewardDefinition.h"
#include "GamePlay/Items/ItemCursedSword.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Sound/SoundBase.h"

UItemEffectComponent::UItemEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UItemManagerComponent* UItemEffectComponent::GetItemManager() const
{
	UItemEffectComponent* MutableThis = const_cast<UItemEffectComponent*>(this);
	if (!MutableThis->CachedItemManager && MutableThis->GetOwner())
	{
		MutableThis->CachedItemManager = MutableThis->GetOwner()->FindComponentByClass<UItemManagerComponent>();
	}
	return MutableThis->CachedItemManager;
}

UMaliceComponent* UItemEffectComponent::GetMaliceComponent() const
{
	UItemEffectComponent* MutableThis = const_cast<UItemEffectComponent*>(this);
	if (!MutableThis->CachedMaliceComponent && MutableThis->GetOwner())
	{
		MutableThis->CachedMaliceComponent = MutableThis->GetOwner()->FindComponentByClass<UMaliceComponent>();
	}
	return MutableThis->CachedMaliceComponent;
}

UCharacterRewardComponent* UItemEffectComponent::GetRewardComponent() const
{
	UItemEffectComponent* MutableThis = const_cast<UItemEffectComponent*>(this);
	if (!MutableThis->CachedRewardComponent && MutableThis->GetOwner())
	{
		MutableThis->CachedRewardComponent = MutableThis->GetOwner()->FindComponentByClass<UCharacterRewardComponent>();
	}
	return MutableThis->CachedRewardComponent;
}

void UItemEffectComponent::BeginPlay()
{
	Super::BeginPlay();

	// ItemManagerComponent 바인딩
	CachedItemManager = GetOwner() ? GetOwner()->FindComponentByClass<UItemManagerComponent>() : nullptr;
	if (CachedItemManager)
	{
		CachedItemManager->DelegateAddItem.RemoveAll(this);
		CachedItemManager->DelegateAddItem.AddUObject(this, &UItemEffectComponent::OnItemAdded);
		
		UE_LOG(LogTemp, Warning,
			TEXT("[ItemEffectComponent] ✓ Bound to ItemManager (Owner: %s, Item Effects: %d)"),
			*GetNameSafe(GetOwner()),
			ItemEffects.Num());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ItemEffectComponent] ItemManagerComponent not found"));
	}

	// MaliceComponent 바인딩
	CachedMaliceComponent = GetOwner() ? GetOwner()->FindComponentByClass<UMaliceComponent>() : nullptr;
	if (CachedMaliceComponent)
	{
		CachedMaliceComponent->OnMaliceChanged.RemoveDynamic(this, &UItemEffectComponent::OnMaliceChanged);
		CachedMaliceComponent->OnMaliceChanged.AddDynamic(this, &UItemEffectComponent::OnMaliceChanged);
		
		UE_LOG(LogTemp, Warning,
			TEXT("[ItemEffectComponent] ✓ Bound to MaliceComponent (Owner: %s, Malice Effects: %d)"),
			*GetNameSafe(GetOwner()),
			MaliceEffects.Num());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ItemEffectComponent] MaliceComponent not found"));
	}

	// CharacterRewardComponent 바인딩 (모든 Reward 타입 감지)
	CachedRewardComponent = GetOwner() ? GetOwner()->FindComponentByClass<UCharacterRewardComponent>() : nullptr;
	if (CachedRewardComponent)
	{
		CachedRewardComponent->OnRewardAcquired.RemoveAll(this);
		CachedRewardComponent->OnRewardAcquired.AddUObject(this, &UItemEffectComponent::OnRewardAcquired);
		
		UE_LOG(LogTemp, Warning,
			TEXT("[ItemEffectComponent] ✓ Bound to CharacterRewardComponent (Owner: %s)"),
			*GetNameSafe(GetOwner()));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ItemEffectComponent] CharacterRewardComponent not found"));
	}
}

void UItemEffectComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CachedItemManager)
	{
		CachedItemManager->DelegateAddItem.RemoveAll(this);
	}

	if (CachedMaliceComponent)
	{
		CachedMaliceComponent->OnMaliceChanged.RemoveDynamic(this, &UItemEffectComponent::OnMaliceChanged);
	}

	if (CachedRewardComponent)
	{
		CachedRewardComponent->OnRewardAcquired.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

void UItemEffectComponent::OnItemAdded(int32 SlotIndex, const UItemDefinition* ItemDefinition)
{
	if (!ItemDefinition)
	{
		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[ItemEffectComponent] >>> Item Added – ItemID='%s', Slot=%d"),
		*ItemDefinition->ItemId.ToString(),
		SlotIndex);

	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		if (OwnerPawn->IsLocallyControlled())
		{
			if (AMyPlayerController* PlayerController = Cast<AMyPlayerController>(OwnerPawn->GetController()))
			{
				const UClass* LogicClass = ItemDefinition->ResolveRewardLogicClass();
				const bool bIsCursedSword = LogicClass && LogicClass->IsChildOf(UItemCursedSword::StaticClass());
				const FText ItemName = ItemDefinition->DisplayName.IsEmpty()
					? FText::FromName(ItemDefinition->ItemId)
					: ItemDefinition->DisplayName;
				const FText Description = ItemDefinition->Description;

				if (!bIsCursedSword && (!ItemName.IsEmpty() || !Description.IsEmpty()))
				{
					PlayerController->ShowItemDescription(ItemName, Description, 4.0f);
				}
			}
		}
	}

	// ItemEffects 배열에서 일치하는 ItemID 찾기
	for (const FItemEffectData& EffectData : ItemEffects)
	{
		if (EffectData.ItemID == ItemDefinition->ItemId)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ItemEffectComponent] !!! Match found for ItemID='%s'"), *EffectData.ItemID.ToString());
			PlayItemEffect(EffectData);
			return;
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("[ItemEffectComponent] No effect configured for ItemID='%s'"),
		*ItemDefinition->ItemId.ToString());
}

void UItemEffectComponent::OnMaliceChanged(int32 NewCount)
{
	UE_LOG(LogTemp, Warning,
		TEXT("[ItemEffectComponent] >>> Malice Changed – NewCount=%d"),
		NewCount);

	// MaliceEffects 배열에서 일치하는 MaliceCount 찾기
	for (const FMaliceEffectData& EffectData : MaliceEffects)
	{
		if (EffectData.MaliceCount == NewCount)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ItemEffectComponent] !!! Match found for MaliceCount=%d"), NewCount);
			PlayMaliceEffect(EffectData, NewCount);
			return;
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("[ItemEffectComponent] No effect configured for MaliceCount=%d"),
		NewCount);
}

// [추가] Reward 획득 핸들러 (아이템과 이벤트 모두 처리)
void UItemEffectComponent::OnRewardAcquired(const URewardDefinition* RewardDefinition)
{
	if (!RewardDefinition)
	{
		return;
	}

	FName RewardID = RewardDefinition->ItemId;
	UE_LOG(LogTemp, Warning,
		TEXT("[ItemEffectComponent] >>> Reward Acquired – ItemID='%s', Category=%d"),
		*RewardID.ToString(),
		static_cast<int32>(RewardDefinition->RewardCategory));

	// ItemEffects 배열에서 매칭되는 ItemID 찾기
	for (const FItemEffectData& EffectData : ItemEffects)
	{
		if (EffectData.ItemID == RewardID)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[ItemEffectComponent] !!! Match found for ItemID='%s'"), *RewardID.ToString());
			PlayItemEffect(EffectData);
			return;
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[ItemEffectComponent] No matching effect configured for ItemID='%s'"), *RewardID.ToString());
}

void UItemEffectComponent::PlayItemEffect(const FItemEffectData& EffectData)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		UE_LOG(LogTemp, Error, TEXT("[ItemEffectComponent] PlayItemEffect – Owner is NULL"));
		return;
	}

	// 로컬 플레이어만 효과 재생
	APawn* OwnerPawn = Cast<APawn>(Owner);
	if (OwnerPawn && !OwnerPawn->IsLocallyControlled())
	{
		UE_LOG(LogTemp, Log, TEXT("[ItemEffectComponent] Skipping item effect (not locally controlled)"));
		return;
	}

	const FVector SpawnLocation = Owner->GetActorLocation();

	// VFX 재생
	if (EffectData.VFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			EffectData.VFX,
			SpawnLocation
		);
		UE_LOG(LogTemp, Warning, TEXT("[ItemEffectComponent] ✓ VFX spawned: %s"), *GetNameSafe(EffectData.VFX));
	}

	// Sound 재생
	if (EffectData.Sound)
	{
		if (EffectData.Sound->IsLooping())
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[ItemEffectComponent] Looping sound '%s' is not valid for one-shot item effect (ItemID='%s')."),
				*GetNameSafe(EffectData.Sound),
				*EffectData.ItemID.ToString()
			);
		}
		else
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				EffectData.Sound,
				SpawnLocation
			);
			UE_LOG(LogTemp, Warning, TEXT("[ItemEffectComponent] ✓ Sound played: %s"), *GetNameSafe(EffectData.Sound));
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[ItemEffectComponent] Item effect completed – ItemID='%s'"), *EffectData.ItemID.ToString());
}

void UItemEffectComponent::PlayMaliceEffect(const FMaliceEffectData& EffectData, int32 MaliceCount)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		UE_LOG(LogTemp, Error, TEXT("[ItemEffectComponent] PlayMaliceEffect – Owner is NULL"));
		return;
	}

	// 로컬 플레이어만 효과 재생
	APawn* OwnerPawn = Cast<APawn>(Owner);
	if (OwnerPawn && !OwnerPawn->IsLocallyControlled())
	{
		UE_LOG(LogTemp, Log, TEXT("[ItemEffectComponent] Skipping malice effect (not locally controlled)"));
		return;
	}

	const FVector SpawnLocation = Owner->GetActorLocation();

	// VFX 재생
	if (EffectData.VFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			EffectData.VFX,
			SpawnLocation
		);
		UE_LOG(LogTemp, Warning, TEXT("[ItemEffectComponent] ✓ VFX spawned: %s"), *GetNameSafe(EffectData.VFX));
	}

	// Sound 재생
	if (EffectData.Sound)
	{
		if (EffectData.Sound->IsLooping())
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[ItemEffectComponent] Looping sound '%s' is not valid for one-shot malice effect (MaliceCount=%d)."),
				*GetNameSafe(EffectData.Sound),
				MaliceCount
			);
		}
		else
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				EffectData.Sound,
				SpawnLocation
			);
			UE_LOG(LogTemp, Warning, TEXT("[ItemEffectComponent] ✓ Sound played: %s"), *GetNameSafe(EffectData.Sound));
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[ItemEffectComponent] Malice effect completed – MaliceCount=%d"), MaliceCount);
}
