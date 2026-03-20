// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GameMode/A302GameState.h"
#include "Interface/A302ServerRewardBridge.h"
#include "A302GameMode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInGameChatReceived, const FString &, PlayerName, const FString &, Message);

class AActor;
class ACharacter;
class FJsonObject;
struct FUniqueNetIdRepl;
class UGameServerBackendSubsystem;
class URewardDefinition;
class URoomMembershipRegistry;
class UA302RoomRuntimeSubsystem;
class UA302ServerPhaseSubsystem;
class UA302ServerBackendRouter;

UCLASS()
class A302SERVER_API AA302GameMode : public AGameMode, public IA302ServerRewardBridge
{
    GENERATED_BODY()

public:
    AA302GameMode();

    UPROPERTY(EditDefaultsOnly, Category = "Spawn")
    TSubclassOf<ACharacter> CharacterClass;

    UPROPERTY(BlueprintAssignable)
    FOnInGameChatReceived OnInGameChatReceived;

    virtual bool TryHandlePersonalEventReward(ACharacter* InstigatorCharacter, AActor* InteractedActor, const URewardDefinition* RewardDefinition) override;
    virtual bool TryHandleGroupEventReward(ACharacter* InstigatorCharacter, AActor* InteractedActor, const URewardDefinition* RewardDefinition) override;
    virtual void NotifyInteractionRewardResolved(ACharacter* InstigatorCharacter, const URewardDefinition* RewardDefinition, ERewardCategory EffectiveCategory) override;
    virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;

    void SpawnPlayersInRoom(const FString& RoomCode);
    void StartRoomGameplay(const FString& RoomCode);
    bool IsRoomGameplayActive(const FString& RoomCode) const;

    UFUNCTION(BlueprintCallable, Category = "Room")
    virtual void PostLogin(APlayerController *NewPlayer) override;
    virtual void Logout(AController *Exiting) override;
    virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
    virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal) override;

    UFUNCTION(BlueprintCallable, Category = "Room")
    URoomMembershipRegistry* GetRoomMembershipRegistry() const { return RoomMembershipRegistry; }
    UGameServerBackendSubsystem* GetBackendSubsystem() const { return BackendSubsystem; }

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    TObjectPtr<class ASpawnManager> SpawnManager;

    UPROPERTY()
    TObjectPtr<UGameServerBackendSubsystem> BackendSubsystem;

    UPROPERTY(Transient)
    TObjectPtr<URoomMembershipRegistry> RoomMembershipRegistry;

    UPROPERTY(Transient)
    TObjectPtr<UA302ServerPhaseSubsystem> PhaseSubsystem;

    UPROPERTY(Transient)
    TObjectPtr<UA302RoomRuntimeSubsystem> RoomRuntimeSubsystem;

    UPROPERTY(Transient)
    TObjectPtr<UA302ServerBackendRouter> BackendRouter;

    UPROPERTY(Config, EditAnywhere, Category = "Room Runtime")
    float ClientRoomStreamWarmupSeconds = 1.0f;

    UFUNCTION()
    void OnMessageReceived(const FString &Message);

    UFUNCTION()
    void HandleRoomPhaseChanged(const FString& RoomCode, EGamePhase NewPhase);
    void HandleRoomLevelReady(const FString& RoomCode);


    int CurrentStage = 1;

    bool EnsureSpawnManager();
};
