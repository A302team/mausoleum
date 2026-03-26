#pragma once

#include "CoreMinimal.h"

class ACharacter;
class URewardDefinition;
enum class ERewardCategory : uint8;

DECLARE_MULTICAST_DELEGATE_ThreeParams(
	FA302ServerRewardResolvedSignal,
	ACharacter* /*InstigatorCharacter*/,
	const URewardDefinition* /*RewardDefinition*/,
	ERewardCategory /*EffectiveCategory*/
);

/**
 * Shared fallback signal for reward resolution when the active GameMode
 * does not implement IA302ServerRewardBridge.
 */
A302SHARED_API FA302ServerRewardResolvedSignal& A302GetServerRewardResolvedSignal();

