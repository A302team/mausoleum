#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Voice/VoiceChatMode.h"
#include "VoiceChatStrategyBase.generated.h"

class UPrivateVoiceChatComponent;

UCLASS(Abstract)
class A302_API UVoiceChatStrategyBase : public UObject
{
    GENERATED_BODY()

public:
    virtual EVoiceChatMode GetMode() const PURE_VIRTUAL(UVoiceChatStrategyBase::GetMode, return EVoiceChatMode::Lobby;);
    virtual bool CanReceiveVoice(const UPrivateVoiceChatComponent* ListenerComp, const UPrivateVoiceChatComponent* SpeakerComp) const PURE_VIRTUAL(UVoiceChatStrategyBase::CanReceiveVoice, return false;);
};
