// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "SnowSimulationActor.generated.h"

USTRUCT()
struct SNOWSIMULATION_API FLandscapeCell
{
	GENERATED_BODY()

	UPROPERTY()
	FVector P1;

	UPROPERTY()
	FVector P2;

	UPROPERTY()
	FVector Normal;

	FLandscapeCell() : P1(FVector::ZeroVector), P2(FVector::ZeroVector), Normal(FVector::ZeroVector) {}

	FLandscapeCell(FVector& p1, FVector& p2, FVector& normal) : P1(p1), P2(p2), Normal(normal) {}
	
};

UCLASS()
class SNOWSIMULATION_API ASnowSimulationActor : public AActor
{
	GENERATED_BODY()
	
public:	
	UPROPERTY()
	TArray<FLandscapeCell> LandscapeCells;

	// Sets default values for this actor's properties
	ASnowSimulationActor();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	/*
	* Creates the cells for the simulation.
	*/
	void CreateCells();
	
};
