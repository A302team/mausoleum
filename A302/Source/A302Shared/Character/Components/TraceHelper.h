#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

namespace TraceHelper
{
	FORCEINLINE bool TryGetCrosshairTrace(ACharacter* OwnerCharacter, FVector& OutStart, FVector& OutDirection)
	{
		OutStart = FVector::ZeroVector;
		OutDirection = FVector::ForwardVector;

		if (!OwnerCharacter)
		{
			return false;
		}

		APlayerController* PlayerController = Cast<APlayerController>(OwnerCharacter->GetController());
		if (!PlayerController || !PlayerController->IsLocalController())
		{
			return false;
		}

		int32 ViewportSizeX = 0;
		int32 ViewportSizeY = 0;
		PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
		if (ViewportSizeX <= 0 || ViewportSizeY <= 0)
		{
			return false;
		}

		FVector WorldOrigin = FVector::ZeroVector;
		FVector WorldDirection = FVector::ForwardVector;
		const bool bDidDeproject = PlayerController->DeprojectScreenPositionToWorld(
			ViewportSizeX * 0.5f,
			ViewportSizeY * 0.5f,
			WorldOrigin,
			WorldDirection
		);
		if (!bDidDeproject)
		{
			return false;
		}

		OutStart = WorldOrigin;
		OutDirection = WorldDirection.GetSafeNormal();
		return true;
	}
}
