#include "Voice/Strategy/LobbyVoiceChatStrategy.h"

#include "Voice/PrivateVoiceChatComponent.h"

bool ULobbyVoiceChatStrategy::CanReceiveVoice(const UPrivateVoiceChatComponent* ListenerComp, const UPrivateVoiceChatComponent* SpeakerComp) const
{
    if (!ListenerComp || !SpeakerComp)
    {
        return false;
    }

    // 임시: RoomCode 검사 로직 제거 (현재는 같은 씬 내에 있으면 모두 들리게)
    // const FString& ListenerRoom = ListenerComp->GetRoomCode();
    // const FString& SpeakerRoom = SpeakerComp->GetRoomCode();
    //
    // if (ListenerRoom.IsEmpty() || SpeakerRoom.IsEmpty())
    // {
    //     return false;
    // }
    //
    // return ListenerRoom == SpeakerRoom;

    return true;
}
