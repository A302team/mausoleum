#include "Object/ItemSpawnArea.h"

bool AItemSpawnArea::IsPhaseAllowed(EGamePhase InPhase) const
{
	if (AllowedPhases.Num() > 0)
	{
		return AllowedPhases.Contains(InPhase);
	}

	// Backward-compatible fallback:
	// if no explicit phase list is provided, use inherited TargetStage when set.
	// TargetStage: 1->Phase0, 2->Phase1, 3->Phase2.
	// TargetStage가 비어있거나(<=0) 지원 범위 밖이면 비활성 영역으로 처리한다.
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
			return false;
		}
	}

	return false;
}
