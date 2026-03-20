#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Templates/SubclassOf.h"
#include "EquipmentComponent.generated.h"

class AWeaponActor;
class AShieldActor;
class ACharacter;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class A302SHARED_API UEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UEquipmentComponent();

protected:
	virtual void BeginPlay() override;

public:
	// Weapon Actions
	void EquipKnifeWeapon();
	void EquipCursedSwordWeapon();
	void EquipShieldWeapon();
	void PlayShieldBlockPresentation();

	void EquipWeapon(TSubclassOf<AWeaponActor> WeaponClass);
	void ShowWeapon();
	void HideWeapon();

	// Weapon Classes
	UPROPERTY(EditAnywhere, Category="Weapon")
	TSubclassOf<AWeaponActor> KnifeActorClass;

	UPROPERTY(EditAnywhere, Category="Weapon")
	TSubclassOf<AWeaponActor> CursedSwordActorClass;

	UPROPERTY(EditAnywhere, Category="Weapon")
	TSubclassOf<AWeaponActor> ShieldActorClass;

	UPROPERTY(Transient)
	AWeaponActor* CurrentWeaponActor;

	UPROPERTY(Transient)
	AShieldActor* CurrentShield = nullptr;

private:
	ACharacter* GetOwnerCharacter() const;
};
