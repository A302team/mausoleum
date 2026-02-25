// Fill out your copyright notice in the Description page of Project Settings.


#include "Server/Private/MyPlayerController.h"
#include "Net/UnrealNetwork.h"
AMyPlayerController::AMyPlayerController()
{
    // Enable replication for this actor
    bReplicates = true;
}

void AMyPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if(IsLocalController() && HasAuthority() == false)
    {
        UE_LOG(LogTemp, Log, TEXT("This is the local player controller."));
        Server_SendMessage(TEXT("Hello from the client!"));
    }
}

bool AMyPlayerController::Server_SendMessage_Validate(const FString& Message)
{
    // Optionally, you can add validation logic here (e.g., check message length)
    if(Message.Len() > 100)
    {
        UE_LOG(LogTemp, Warning, TEXT("Message is too long!"));
        return false; // Reject the message if it's too long
    }
    return true; // Return true if the message is valid
    
}
void AMyPlayerController::Server_SendMessage_Implementation(const FString& Message)
{
    // Process the message on the server (e.g., log it or update a variable)
    UE_LOG(LogTemp, Log, TEXT("Received message from client: %s"), *Message);
    
    // Update the ServerResponse variable to trigger replication
    ServerResponse = FString::Printf(TEXT("Server received: %s"), *Message);
}

void AMyPlayerController::OnRep_ServerResponse()
{
    // This function will be called on clients when ServerResponse is updated on the server
    UE_LOG(LogTemp, Log, TEXT("Updated Server Response: %s"), *ServerResponse);
}

void AMyPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Replicate the ServerResponse variable to all clients
    DOREPLIFETIME_CONDITION(AMyPlayerController, ServerResponse, COND_OwnerOnly);
}