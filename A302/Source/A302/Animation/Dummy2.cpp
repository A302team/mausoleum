// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/Dummy2.h"

// Sets default values
ADummy2::ADummy2()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ADummy2::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ADummy2::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

