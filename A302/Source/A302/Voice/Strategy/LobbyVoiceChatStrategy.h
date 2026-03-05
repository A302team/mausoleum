#pragma once

#include "CoreMinimal.h"
#include "Voice/Strategy/VoiceChatStrategyBase.h"
#include "LobbyVoiceChatStrategy.generated.h"

UCLASS()
class A302_API ULobbyVoiceChatStrategy : public UVoiceChatStrategyBase
{
    GENERATED_BODY()

public:
    virtual EVoiceChatMode GetMode() const override
    {
        return EVoiceChatMode::Lobby;
    }

    virtual bool CanReceiveVoice(const UPrivateVoiceChatComponent* ListenerComp, const UPrivateVoiceChatComponent* SpeakerComp) const override;
};
