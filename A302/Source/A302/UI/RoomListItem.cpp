// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/RoomListItem.h"
#include "GameMode/A302GameInstance.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

void URoomListItem::NativeConstruct()
{
    Super::NativeConstruct();

    GI = Cast<UA302GameInstance>(UGameplayStatics::GetGameInstance(this));

    if (Btn_Join)
    {
        Btn_Join->OnClicked.AddDynamic(this, &URoomListItem::OnJoinClicked);
    }
}

void URoomListItem::SetRoomInfo(const FString &InRoomCode, int32 PlayerCount)
{
    RoomCode = InRoomCode;
    if (Text_RoomInfo)
    {
        Text_RoomInfo->SetText(
            FText::FromString(FString::Printf(TEXT("%s (%d명)"), *RoomCode, PlayerCount)));
    }
}

void URoomListItem::OnJoinClicked()
{
    if (!GI)
        return;

    FString PlayerName = GI->MyPlayerName;

    if (PlayerName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[UI/RoomListItem] 닉네임을 먼저 입력해주세요!"));
        return;
    }

    TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
    Data->SetStringField(TEXT("roomCode"), RoomCode);
    Data->SetStringField(TEXT("playerName"), PlayerName);

    TSharedPtr<FJsonObject> Json = MakeShareable(new FJsonObject);
    Json->SetStringField(TEXT("type"), TEXT("join_room"));
    Json->SetObjectField(TEXT("data"), Data);

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Json.ToSharedRef(), Writer);

    GI->SendToServer(Output);
    UE_LOG(LogTemp, Log, TEXT("[UI/RoomListItem] 방 입장 시도: %s"), *RoomCode);

    // 팝업 닫기
    RemoveFromParent();
}
