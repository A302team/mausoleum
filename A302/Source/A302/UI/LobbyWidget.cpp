#include "UI/LobbyWidget.h"
#include "UI/EnterRoomPopup.h"
#include "UI/RoomListPopup.h"
#include "GameMode/LobbyGameMode.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

void ULobbyWidget::NativeConstruct()
{
    Super::NativeConstruct();

    LobbyGameMode = Cast<ALobbyGameMode>(UGameplayStatics::GetGameMode(this));

    if (Btn_CreateRoom) {
        Btn_CreateRoom->OnClicked.AddDynamic(this, &ULobbyWidget::OnCreateRoomClicked);
    }
    if (Btn_EnterRoom) {
        Btn_EnterRoom->OnClicked.AddDynamic(this, &ULobbyWidget::OnEnterRoomClicked);
    }
    if (Btn_FindRoom) {
        Btn_FindRoom->OnClicked.AddDynamic(this, &ULobbyWidget::OnFindRoomClicked);
    }
    if (Btn_Exit) {
        Btn_Exit->OnClicked.AddDynamic(this, &ULobbyWidget::OnExitClicked);
    }

    if (LobbyGameMode) {
        LobbyGameMode->OnRoomCreated.AddDynamic(this, &ULobbyWidget::OnRoomCreated);
        LobbyGameMode->OnNicknameAvailable.AddDynamic(this, &ULobbyWidget::OnNicknameAvailable);
    }
}

void ULobbyWidget::CheckNickname(const FString& PlayerName)
{
    TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
    Data->SetStringField(TEXT("playerName"), PlayerName);

    TSharedPtr<FJsonObject> Json = MakeShareable(new FJsonObject);
    Json->SetStringField(TEXT("type"), TEXT("check_nickname"));
    Json->SetObjectField(TEXT("data"), Data);

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Json.ToSharedRef(), Writer);

    LobbyGameMode->SendToServer(Output);
}

void ULobbyWidget::OnNicknameAvailable()
{
    if (PendingAction == EPendingAction::CreateRoom)
    {
        TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
        Data->SetStringField(TEXT("playerName"), LobbyGameMode->MyPlayerName);

        TSharedPtr<FJsonObject> Json = MakeShareable(new FJsonObject);
        Json->SetStringField(TEXT("type"), TEXT("create_room"));
        Json->SetObjectField(TEXT("data"), Data);

        FString Output;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
        FJsonSerializer::Serialize(Json.ToSharedRef(), Writer);

        LobbyGameMode->SendToServer(Output);
        UE_LOG(LogTemp, Log, TEXT("[UI/LobbyWidget] Create Room"));
    }
    else if (PendingAction == EPendingAction::EnterRoom)
    {
        FString RoomCode = Input_RoomCode->GetText().ToString();

        TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
        Data->SetStringField(TEXT("roomCode"), RoomCode);
        Data->SetStringField(TEXT("playerName"), LobbyGameMode->MyPlayerName);

        TSharedPtr<FJsonObject> Json = MakeShareable(new FJsonObject);
        Json->SetStringField(TEXT("type"), TEXT("join_room"));
        Json->SetObjectField(TEXT("data"), Data);

        FString Output;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
        FJsonSerializer::Serialize(Json.ToSharedRef(), Writer);

        LobbyGameMode->SendToServer(Output);
        UE_LOG(LogTemp, Log, TEXT("[UI/LobbyWidget] Enter Room - Code: %s"), *RoomCode);
    }
    else if (PendingAction == EPendingAction::FindRoom)
    {
        if (RoomListPopupClass) {
            URoomListPopup* Popup = CreateWidget<URoomListPopup>(GetWorld(), RoomListPopupClass);
            if (Popup) {
                Popup->AddToViewport();
                UE_LOG(LogTemp, Log, TEXT("[UI/LobbyWidget] Open room list popup"));
            }
        }
    }

    PendingAction = EPendingAction::None;
}

void ULobbyWidget::OnCreateRoomClicked()
{
    FString PlayerName = Input_PlayerName->GetText().ToString();

    if (PlayerName.IsEmpty()) {
        UE_LOG(LogTemp, Warning, TEXT("[UI/LobbyWidget] 닉네임을 입력해주세요!"));
        return;
    }

    PendingAction = EPendingAction::CreateRoom;
    LobbyGameMode->MyPlayerName = PlayerName;
    CheckNickname(PlayerName);
}

void ULobbyWidget::OnEnterRoomClicked()
{
    FString PlayerName = Input_PlayerName->GetText().ToString();
    FString RoomCode = Input_RoomCode->GetText().ToString();

    if (PlayerName.IsEmpty()) {
        UE_LOG(LogTemp, Warning, TEXT("[UI/LobbyWidget] 닉네임을 입력해주세요!"));
        return;
    }
    if (RoomCode.IsEmpty()) {
        UE_LOG(LogTemp, Warning, TEXT("[UI/LobbyWidget] 방 코드를 입력해주세요!"));
        return;
    }

    PendingAction = EPendingAction::EnterRoom;
    LobbyGameMode->MyPlayerName = PlayerName;
    CheckNickname(PlayerName);
}

void ULobbyWidget::OnFindRoomClicked()
{
    FString PlayerName = Input_PlayerName->GetText().ToString();

    if (PlayerName.IsEmpty()) {
        UE_LOG(LogTemp, Warning, TEXT("[UI/LobbyWidget] 닉네임을 입력해주세요!"));
        return;
    }

    PendingAction = EPendingAction::FindRoom;
    LobbyGameMode->MyPlayerName = PlayerName;
    CheckNickname(PlayerName);
}

void ULobbyWidget::OnExitClicked()
{
    UE_LOG(LogTemp, Log, TEXT("[UI/LobbyWidget] Exit"));
    UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
}

void ULobbyWidget::OnRoomCreated(const FString& RoomCode)
{
    if (Input_RoomCode) {
        Input_RoomCode->SetText(FText::FromString(RoomCode));
        UE_LOG(LogTemp, Log, TEXT("[UI/LobbyWidget] Room Code : %s"), *RoomCode);
    }
}