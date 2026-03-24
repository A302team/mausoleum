#include "GameData/Spawn/ItemSpawnPolicy.h"

const FPhaseItemSpawnPolicy* UItemSpawnPolicy::FindPhasePolicy(EGamePhase Phase) const
{
	for (const FPhaseItemSpawnPolicy& Policy : PhasePolicies)
	{
		if (Policy.Phase == Phase)
		{
			return &Policy;
		}
	}

	return nullptr;
}

