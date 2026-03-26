#include "Voice/Strategy/DistanceVoiceChatStrategy.h"

#include "Voice/PrivateVoiceChatComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameMode/A302PlayerState.h"

namespace
{
    const AA302PlayerState* ResolveA302PlayerState(const UPrivateVoiceChatComponent* VoiceComponent)
    {
        const APawn* OwnerPawn = VoiceComponent ? Cast<APawn>(VoiceComponent->GetOwner()) : nullptr;
        return OwnerPawn ? OwnerPawn->GetPlayerState<AA302PlayerState>() : nullptr;
    }
}

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

    const AA302PlayerState* ListenerPlayerState = ResolveA302PlayerState(ListenerComp);
    const AA302PlayerState* SpeakerPlayerState = ResolveA302PlayerState(SpeakerComp);
    const bool bListenerDead = ListenerPlayerState && !ListenerPlayerState->bIsAlive;
    const bool bSpeakerDead = SpeakerPlayerState && !SpeakerPlayerState->bIsAlive;

    if (bListenerDead || bSpeakerDead)
    {
        // 사망자는 사망자끼리만 보이스 허용 (거리 제한 없음)
        return bListenerDead && bSpeakerDead;
    }

    const float DistSquared = FVector::DistSquared(ListenerActor->GetActorLocation(), SpeakerActor->GetActorLocation());
    return DistSquared <= FMath::Square(HearingDistance);
}

void UDistanceVoiceChatStrategy::SetHearingDistance(float InDistance)
{
    HearingDistance = FMath::Max(100.f, InDistance);
}
