#include "Interface/A302ServerRewardSignals.h"

namespace
{
	FA302ServerRewardResolvedSignal GServerRewardResolvedSignal;
}

FA302ServerRewardResolvedSignal& A302GetServerRewardResolvedSignal()
{
	return GServerRewardResolvedSignal;
}

