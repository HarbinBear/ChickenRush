// Fill out your copyright notice in the Description page of Project Settings.


#include "ChickenBall.h"

#include "Projects.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"


// Sets default values
AChickenBall::AChickenBall():
	bHolded( false )
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GetStaticMeshComponent()->Mobility = EComponentMobility::Movable;
	GetStaticMeshComponent()->SetSimulatePhysics(true);
	// GetStaticMeshComponent()->OnComponentHit.AddDynamic(this, &AChickenBall::OnBallHit);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->UpdatedComponent = GetStaticMeshComponent();

	
}

void AChickenBall::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// DOREPLIFETIME( AChickenBall , bHolded );
}

void AChickenBall::ThrowBall(const FVector& Direction)
{
	GetStaticMeshComponent()->SetSimulatePhysics(false);
	GetStaticMeshComponent()->SetEnableGravity(false);
	GetStaticMeshComponent()->SetCollisionEnabled( ECollisionEnabled::QueryAndPhysics );
	// GetStaticMeshComponent()->SetCollisionResponseToChannel( ECC_Pawn , ECR_Ignore);

	ProjectileMovementComponent->ProjectileGravityScale = 0.0f;
	ProjectileMovementComponent->InitialSpeed = 1500.0f;
	ProjectileMovementComponent->MaxSpeed = 1500.0f;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->bShouldBounce = true;
	ProjectileMovementComponent->Velocity = Direction * 1500.0f ; 
	
	ProjectileMovementComponent->Activate();

	FDetachmentTransformRules Rules( EDetachmentRule::KeepWorld , true );
	DetachFromActor( Rules );
	bHolded = false;

	UKismetSystemLibrary::PrintString(GetWorld(),TEXT("Ball Throw Ball, Velocity: ")
		+ ProjectileMovementComponent->Velocity.ToString()  );
}

void AChickenBall::OnBallHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	GetStaticMeshComponent()->SetSimulatePhysics(true);
	GetStaticMeshComponent()->SetEnableGravity(true);
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

