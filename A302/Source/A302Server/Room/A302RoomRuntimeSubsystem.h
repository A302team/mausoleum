#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "A302RoomRuntimeSubsystem.generated.h"

class ULevelStreamingDynamic;

USTRUCT()
struct FA302RoomRuntimeState
{
    GENERATED_BODY()

    UPROPERTY()
    FString RoomCode;

    UPROPERTY()
    FVector RoomOffset = FVector::ZeroVector;

    UPROPERTY()
    TObjectPtr<ULevelStreamingDynamic> StreamingLevel = nullptr;

    UPROPERTY()
    bool bLevelInstanceRequested = false;

    UPROPERTY()
    bool bLevelReady = false;

    UPROPERTY()
    double LastTouchedServerTime = 0.0;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnRoomLevelReadyNative, const FString& /*RoomCode*/);

UCLASS(Config=Game)
class A302SERVER_API UA302RoomRuntimeSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    bool PrepareRoom(const FString& RoomCode);
    FVector ResolveRoomOffset(const FString& RoomCode) const;
    void TouchRoom(const FString& RoomCode);
    bool StopRoomRuntime(const FString& RoomCode);

    void SetRoomPopulationResolver(TFunction<int32(const FString&)>&& InResolver);

    FOnRoomLevelReadyNative& OnRoomLevelReady() { return RoomLevelReadyDelegate; }

private:
    void TickRoomRuntimes();
    void EnsureTickTimer();
    UWorld* ResolveWorld() const;
    bool TryRequestLevelInstance(FA302RoomRuntimeState& RoomState);
    bool ShouldCollectZombieRoom(const FA302RoomRuntimeState& RoomState, double CurrentServerTime) const;

private:
    UPROPERTY(Config, EditAnywhere, Category="Room Runtime")
    FString TemplateLevelPath = TEXT("/Game/PersonalWorkSpace/wjtmd28/MyMap");

    UPROPERTY(Config, EditAnywhere, Category="Room Runtime")
    float RuntimePollInterval = 0.5f;

    UPROPERTY(Config, EditAnywhere, Category="Room Runtime")
    float ZombieRoomTimeoutSeconds = 120.0f;

    UPROPERTY()
    TMap<FString, FA302RoomRuntimeState> RoomRuntimeStates;

    FOnRoomLevelReadyNative RoomLevelReadyDelegate;
    FTimerHandle RuntimeTickTimerHandle;
    TFunction<int32(const FString&)> RoomPopulationResolver;
};
