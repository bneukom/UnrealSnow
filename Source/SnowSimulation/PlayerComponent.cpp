// Fill out your copyright notice in the Description page of Project Settings.

#include "SnowSimulation.h"
#include "PlayerComponent.h"


// Sets default values for this component's properties
UPlayerComponent::UPlayerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UPlayerComponent::BeginPlay()
{
	Super::BeginPlay();

	auto component = GetOwner()->FindComponentByClass<UFloatingPawnMovement>();

	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::FromInt(component != nullptr));
}


// Called every frame
void UPlayerComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );

	// ...
}

