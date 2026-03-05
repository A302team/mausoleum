#pragma once

#include "CoreMinimal.h"
#include "Voice/Strategy/VoiceChatStrategyBase.h"
#include "DistanceVoiceChatStrategy.generated.h"

UCLASS()
class A302_API UDistanceVoiceChatStrategy : public UVoiceChatStrategyBase
{
    GENERATED_BODY()

public:
    virtual EVoiceChatMode GetMode() const override
    {
        return EVoiceChatMode::InGameDistance;
    }

    virtual bool CanReceiveVoice(const UPrivateVoiceChatComponent* ListenerComp, const UPrivateVoiceChatComponent* SpeakerComp) const override;

    void SetHearingDistance(float InDistance);
    float GetHearingDistance() const
    {
        return HearingDistance;
    }

private:
    UPROPERTY()
    float HearingDistance = 1800.f;
};
