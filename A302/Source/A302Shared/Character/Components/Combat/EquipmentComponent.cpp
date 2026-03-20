#include "Character/Components/Combat/EquipmentComponent.h"
#include "GameFramework/Character.h"
#include "GamePlay/Actor/WeaponActor.h"
#include "Interface/A302AnimationBridge.h"
#include "Engine/World.h"

UEquipmentComponent::UEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	CurrentWeaponActor = nullptr;
	CurrentShield = nullptr;
}

void UEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
}

ACharacter* UEquipmentComponent::GetOwnerCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}

void UEquipmentComponent::EquipKnifeWeapon()
{
	if (KnifeActorClass)
	{
		EquipWeapon(KnifeActorClass);
	}
}

void UEquipmentComponent::EquipCursedSwordWeapon()
{
	if (CursedSwordActorClass)
	{
		EquipWeapon(CursedSwordActorClass);
	}
}

void UEquipmentComponent::EquipShieldWeapon()
{
	if (ShieldActorClass)
	{
		EquipWeapon(ShieldActorClass);
	}
}

void UEquipmentComponent::PlayShieldBlockPresentation()
{
	if (ACharacter* OwnerCharacter = GetOwnerCharacter())
	{
		if (IA302AnimationBridge* Anim = Cast<IA302AnimationBridge>(OwnerCharacter->GetMesh()->GetAnimInstance()))
		{
			Anim->PlayBlockAnimation();
		}
	}
}

void UEquipmentComponent::EquipWeapon(TSubclassOf<AWeaponActor> WeaponClass)
{
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter) return;

	UE_LOG(LogTemp, Warning, TEXT("EquipWeapon called in EquipmentComponent"));

	if (!WeaponClass)
	{
		UE_LOG(LogTemp, Error, TEXT("WeaponClass is NULL"));
		return;
	}

	if (CurrentWeaponActor)
	{
		CurrentWeaponActor->Destroy();
		CurrentWeaponActor = nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("World is NULL"));
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = OwnerCharacter;
	Params.Instigator = OwnerCharacter;

	CurrentWeaponActor = World->SpawnActor<AWeaponActor>(
		WeaponClass,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		Params
	);

	if (!CurrentWeaponActor)
	{
		UE_LOG(LogTemp, Error, TEXT("Weapon Spawn FAILED"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Weapon Spawn SUCCESS"));

	FName SocketName = TEXT("HandGrip_R");

	if (WeaponClass == ShieldActorClass)
	{
		SocketName = TEXT("HandGrip_L");
	}

	CurrentWeaponActor->AttachToComponent(
		OwnerCharacter->GetMesh(),
		FAttachmentTransformRules::SnapToTargetIncludingScale,
		SocketName
	);

	CurrentWeaponActor->ShowWeapon();
}

void UEquipmentComponent::ShowWeapon()
{
	UE_LOG(LogTemp, Warning, TEXT("ShowWeapon called in EquipmentComponent"));
	if (CurrentWeaponActor)
	{
		CurrentWeaponActor->ShowWeapon();
	}
}

void UEquipmentComponent::HideWeapon()
{
	if (CurrentWeaponActor)
	{
		CurrentWeaponActor->HideWeapon();
	}
}
