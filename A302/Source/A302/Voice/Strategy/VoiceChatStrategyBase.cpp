#include "Voice/Strategy/VoiceChatStrategyBase.h"

#include "Voice/PrivateVoiceChatComponent.h"

FString UVoiceChatStrategyBase::GetServerChannelName(const UPrivateVoiceChatComponent* OwnerComp) const
{
    if (!OwnerComp)
    {
        return TEXT("");
    }

    return OwnerComp->GetRoomCode();
}
