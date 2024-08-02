// Fill out your copyright notice in the Description page of Project Settings.


#include "ChickenBall.h"

#include "Net/UnrealNetwork.h"


// Sets default values
AChickenBall::AChickenBall():
	bHolded( false )
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	GetStaticMeshComponent()->Mobility = EComponentMobility::Movable;
	GetStaticMeshComponent()->bReplicatePhysicsToAutonomousProxy = 0;
	SetReplicateMovement(true);
	GetStaticMeshComponent()->SetSimulatePhysics(true);
}

void AChickenBall::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// DOREPLIFETIME( AChickenBall , bHolded );
}

// Called when the game starts or when spawned
void AChickenBall::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AChickenBall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

