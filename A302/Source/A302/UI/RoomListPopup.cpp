// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/RoomListPopup.h"
#include "UI/RoomListItem.h"
#include "GameMode/LobbyGameMode.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

void URoomListPopup::NativeConstruct()
{
    Super::NativeConstruct();

    LobbyGameMode = Cast<ALobbyGameMode>(UGameplayStatics::GetGameMode(this));

    if (Btn_Refresh)
    {
        Btn_Refresh->OnClicked.AddDynamic(this, &URoomListPopup::OnRefreshClicked);
    }
    if (Btn_Close)
    {
        Btn_Close->OnClicked.AddDynamic(this, &URoomListPopup::OnCloseClicked);
    }

    // 방 목록 수신 델리게이트 바인딩
    if (LobbyGameMode)
    {
        LobbyGameMode->OnRoomListReceived.AddDynamic(this, &URoomListPopup::UpdateRoomList);
    }

    // 팝업 열리면 바로 방 목록 요청
    OnRefreshClicked();
}

void URoomListPopup::NativeDestruct()
{
    Super::NativeDestruct();

    if (LobbyGameMode)
    {
        LobbyGameMode->OnRoomListReceived.RemoveDynamic(this, &URoomListPopup::UpdateRoomList);
    }
}

void URoomListPopup::UpdateRoomList(const TArray<FRoomInfo>& RoomList)
{
    if (!ScrollBox_RoomList) return;

    ScrollBox_RoomList->ClearChildren();

    if (RoomList.Num() == 0) {
        UE_LOG(LogTemp, Log, TEXT("[UI/RoomListPopup] 방이 없습니다."));
        return;
    }

    for (auto& Info : RoomList)
    {
        if (RoomListItemClass)
        {
            URoomListItem* Item = CreateWidget<URoomListItem>(GetWorld(), RoomListItemClass);
            if (Item)
            {
                ScrollBox_RoomList->AddChild(Item);
                Item->SetRoomInfo(Info.RoomCode, Info.PlayerCount);
            }
        }
    }
}

void URoomListPopup::OnRefreshClicked()
{
    if (!LobbyGameMode)
        return;

    TSharedPtr<FJsonObject> Json = MakeShareable(new FJsonObject);
    Json->SetStringField(TEXT("type"), TEXT("get_room_list"));
    Json->SetObjectField(TEXT("data"), MakeShareable(new FJsonObject));

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Json.ToSharedRef(), Writer);

    LobbyGameMode->SendToServer(Output);
    UE_LOG(LogTemp, Log, TEXT("[UI/RoomListPopup] 방 목록 새로고침: %s"), *Output);
}

void URoomListPopup::OnCloseClicked()
{
    RemoveFromParent();
}
