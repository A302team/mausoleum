#include "Voice/Strategy/LobbyVoiceChatStrategy.h"

#include "Character/Components/PrivateVoiceChatComponent.h"

bool ULobbyVoiceChatStrategy::CanReceiveVoice(const UPrivateVoiceChatComponent* ListenerComp, const UPrivateVoiceChatComponent* SpeakerComp) const
{
    if (!ListenerComp || !SpeakerComp)
    {
        return false;
    }

    const FString& ListenerRoom = ListenerComp->GetRoomId();
    const FString& SpeakerRoom = SpeakerComp->GetRoomId();

    if (ListenerRoom.IsEmpty() || SpeakerRoom.IsEmpty())
    {
        return false;
    }

    return ListenerRoom == SpeakerRoom;
}
