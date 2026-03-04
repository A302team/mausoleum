#include "Voice/Strategy/VoiceChatStrategyBase.h"

#include "Character/Components/PrivateVoiceChatComponent.h"

FString UVoiceChatStrategyBase::GetServerChannelName(const UPrivateVoiceChatComponent* OwnerComp) const
{
    if (!OwnerComp)
    {
        return TEXT("");
    }

    return OwnerComp->GetRoomId();
}
