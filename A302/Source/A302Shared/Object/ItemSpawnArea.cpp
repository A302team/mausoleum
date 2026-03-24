#include "Object/ItemSpawnArea.h"

bool AItemSpawnArea::IsPhaseAllowed(EGamePhase InPhase) const
{
	if (AllowedPhases.Num() > 0)
	{
		return AllowedPhases.Contains(InPhase);
	}

	// Backward-compatible fallback:
	// if no explicit phase list is provided, use inherited TargetStage when set.
	// TargetStage: 1->Phase0, 2->Phase1, 3->Phase2, 0(or others)->all phases.
	if (TargetStage > 0)
	{
		switch (InPhase)
		{
		case EGamePhase::Phase0:
			return TargetStage == 1;
		case EGamePhase::Phase1:
			return TargetStage == 2;
		case EGamePhase::Phase2:
			return TargetStage == 3;
		default:
			break;
		}
	}

	return true;
}
