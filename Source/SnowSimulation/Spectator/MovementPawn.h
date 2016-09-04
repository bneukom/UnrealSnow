// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Pawn.h"
#include "MovementPawn.generated.h"

UCLASS()
class SNOWSIMULATION_API AMovementPawn : public ADefaultPawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AMovementPawn();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	
	
};
