#include "Voice/Strategy/DistanceVoiceChatStrategy.h"

#include "Voice/PrivateVoiceChatComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameMode/A302PlayerState.h"

namespace
{
    const AA302PlayerState* ResolveA302PlayerState(const UPrivateVoiceChatComponent* VoiceComponent)
    {
        const APawn* OwnerPawn = VoiceComponent ? Cast<APawn>(VoiceComponent->GetOwner()) : nullptr;
        return OwnerPawn ? OwnerPawn->GetPlayerState<AA302PlayerState>() : nullptr;
    }

    bool IsSpiritPlayer(const AA302PlayerState* PlayerState)
    {
        return PlayerState && (!PlayerState->bIsAlive || PlayerState->bIsEscaped);
    }

    bool IsAlivePlayer(const AA302PlayerState* PlayerState)
    {
        return PlayerState && PlayerState->bIsAlive && !PlayerState->bIsEscaped;
    }

    bool CanAlivePlayersHearSpeaker(const AActor* SpeakerActor, const FString& RoomCode, float HearingDistance)
    {
        if (!SpeakerActor || RoomCode.IsEmpty())
        {
            return false;
        }

        const UWorld* World = SpeakerActor->GetWorld();
        const APawn* SpeakerPawn = Cast<APawn>(SpeakerActor);
        if (!World || !SpeakerPawn)
        {
            return false;
        }

        const float HearingDistanceSquared = FMath::Square(HearingDistance);
        for (TActorIterator<APawn> It(World); It; ++It)
        {
            const APawn* CandidatePawn = *It;
            if (!IsValid(CandidatePawn) || CandidatePawn == SpeakerPawn)
            {
                continue;
            }

            const AA302PlayerState* CandidatePlayerState = CandidatePawn->GetPlayerState<AA302PlayerState>();
            if (!IsAlivePlayer(CandidatePlayerState))
            {
                continue;
            }

            if (CandidatePlayerState->GetRoomCode() != RoomCode)
            {
                continue;
            }

            const float DistSquared = FVector::DistSquared(CandidatePawn->GetActorLocation(), SpeakerActor->GetActorLocation());
            if (DistSquared <= HearingDistanceSquared)
            {
                return true;
            }
        }

        return false;
    }

    bool IsSpeakerCurrentViewTargetForListener(const UPrivateVoiceChatComponent* ListenerComp, const AActor* SpeakerActor)
    {
        const APawn* ListenerPawn = ListenerComp ? Cast<APawn>(ListenerComp->GetOwner()) : nullptr;
        const APlayerController* ListenerPC = ListenerPawn ? Cast<APlayerController>(ListenerPawn->GetController()) : nullptr;
        if (!ListenerPC || !SpeakerActor)
        {
            return false;
        }

        const AActor* CurrentViewTarget = ListenerPC->GetViewTarget();
        return CurrentViewTarget == SpeakerActor;
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
    const bool bListenerSpirit = IsSpiritPlayer(ListenerPlayerState);
    const bool bSpeakerSpirit = IsSpiritPlayer(SpeakerPlayerState);
    const bool bListenerAlive = IsAlivePlayer(ListenerPlayerState);
    const bool bSpeakerAlive = IsAlivePlayer(SpeakerPlayerState);

    if (bListenerAlive)
    {
        // 산 사람은 산 사람끼리만, 거리 기반.
        if (!bSpeakerAlive)
        {
            return false;
        }

        const float DistSquared = FVector::DistSquared(ListenerActor->GetActorLocation(), SpeakerActor->GetActorLocation());
        return DistSquared <= FMath::Square(HearingDistance);
    }

    if (bListenerSpirit)
    {
        // 죽은/탈출한 사람끼리는 거리 제한 없이 대화.
        if (bSpeakerSpirit)
        {
            return true;
        }

        // 죽은/탈출한 사람은 산 사람 음성을 "산 사람 기준으로 실제 들리는 경우"에만 청취.
        if (bSpeakerAlive)
        {
            // 관전 중인 대상(현재 시점 대상) 본인의 발화는 항상 청취 허용.
            if (IsSpeakerCurrentViewTargetForListener(ListenerComp, SpeakerActor))
            {
                return true;
            }

            return CanAlivePlayersHearSpeaker(SpeakerActor, SpeakerRoom, HearingDistance);
        }

        return false;
    }

    // 상태가 아직 미확정인 초기 프레임은 기존 거리 기반으로 보수 처리.
    if (bSpeakerSpirit)
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
