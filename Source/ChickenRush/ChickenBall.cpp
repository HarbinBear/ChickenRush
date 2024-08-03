// Fill out your copyright notice in the Description page of Project Settings.


#include "ChickenBall.h"

#include "ChickenRushCharacter.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"


// Sets default values
AChickenBall::AChickenBall():
	bHolded( false )
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(true);
	SetReplicateMovement(true);

	GetStaticMeshComponent()->Mobility = EComponentMobility::Movable;
	GetStaticMeshComponent()->SetSimulatePhysics(true);
	GetStaticMeshComponent()->OnComponentHit.AddDynamic(this , &AChickenBall::OnBallHit);
	GetStaticMeshComponent()->SetIsReplicated(true);
	GetStaticMeshComponent()->bReplicatePhysicsToAutonomousProxy = 1;


	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->UpdatedComponent = GetStaticMeshComponent();
	ProjectileMovementComponent->OnProjectileBounce.AddDynamic(this, &AChickenBall::OnBallBounce);
	ProjectileMovementComponent->SetIsReplicated(true);
	
	
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


void AChickenBall::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// DOREPLIFETIME( AChickenBall , bHolded );
}

void AChickenBall::PickUpBall()
{
	SetReplicates(false);
	SetReplicateMovement(false);
	
	
	GetStaticMeshComponent()->SetSimulatePhysics(false);
	GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetStaticMeshComponent()->SetEnableGravity(false);
	GetStaticMeshComponent()->SetIsReplicated(false);


	// 禁用ProjectileMovementComponent
	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->Deactivate();
	}
	bHolded = true;
}

void AChickenBall::ThrowBall(const FVector& Direction)
{
	SetReplicates(true);
	SetReplicateMovement(true);
	
	GetStaticMeshComponent()->SetSimulatePhysics(false);
	GetStaticMeshComponent()->SetEnableGravity(false);
	GetStaticMeshComponent()->SetCollisionEnabled( ECollisionEnabled::QueryAndPhysics );
	GetStaticMeshComponent()->SetIsReplicated(true);

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

	UKismetSystemLibrary::PrintString(GetWorld(),TEXT("Ball Throw Ball ") );
}

void AChickenBall::OnBallBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{


	UKismetSystemLibrary::PrintString(GetWorld(),TEXT("On Ball Bounce ") );
	
}

void AChickenBall::OnBallHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	AChickenRushCharacter* crc = Cast<AChickenRushCharacter>(OtherActor);
	// TODO 除了射球时的持球者外，其余物体都应该有碰撞
	if( crc == nullptr )
	{
		GetStaticMeshComponent()->SetSimulatePhysics(true);
		GetStaticMeshComponent()->SetEnableGravity(true);

		ProjectileMovementComponent->Deactivate();
		
		UKismetSystemLibrary::PrintString(GetWorld(),TEXT("On Ball Hit: ") + OtherActor->GetName() );
	}

}



