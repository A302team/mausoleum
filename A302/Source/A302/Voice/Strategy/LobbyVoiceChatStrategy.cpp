#include "Voice/Strategy/LobbyVoiceChatStrategy.h"

#include "Voice/PrivateVoiceChatComponent.h"

bool ULobbyVoiceChatStrategy::CanReceiveVoice(const UPrivateVoiceChatComponent* ListenerComp, const UPrivateVoiceChatComponent* SpeakerComp) const
{
    if (!ListenerComp || !SpeakerComp)
    {
        return false;
    }

    const FString& ListenerRoom = ListenerComp->GetRoomCode();
    const FString& SpeakerRoom = SpeakerComp->GetRoomCode();

    if (ListenerRoom.IsEmpty() || SpeakerRoom.IsEmpty())
    {
        return false;
    }

    return ListenerRoom == SpeakerRoom;
}
