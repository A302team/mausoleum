#include "Voice/Strategy/DistanceVoiceChatStrategy.h"

#include "Character/Components/PrivateVoiceChatComponent.h"
#include "GameFramework/Actor.h"

bool UDistanceVoiceChatStrategy::CanReceiveVoice(const UPrivateVoiceChatComponent* ListenerComp, const UPrivateVoiceChatComponent* SpeakerComp) const
{
    if (!ListenerComp || !SpeakerComp)
    {
        return false;
    }

    const FString& ListenerRoom = ListenerComp->GetRoomCode();
    const FString& SpeakerRoom = SpeakerComp->GetRoomCode();

    if (ListenerRoom.IsEmpty() || SpeakerRoom.IsEmpty() || ListenerRoom != SpeakerRoom)
    {
        return false;
    }

    const AActor* ListenerActor = ListenerComp->GetOwner();
    const AActor* SpeakerActor = SpeakerComp->GetOwner();

    if (!ListenerActor || !SpeakerActor)
    {
        return false;
    }

    const float DistSquared = FVector::DistSquared(ListenerActor->GetActorLocation(), SpeakerActor->GetActorLocation());
    return DistSquared <= FMath::Square(HearingDistance);
}

void UDistanceVoiceChatStrategy::SetHearingDistance(float InDistance)
{
    HearingDistance = FMath::Max(100.f, InDistance);
}
