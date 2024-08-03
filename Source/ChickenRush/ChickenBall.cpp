// Fill out your copyright notice in the Description page of Project Settings.


#include "ChickenBall.h"

#include "ChickenRushCharacter.h"
#include "Components/ArrowComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"


// Sets default values
AChickenBall::AChickenBall():
	bHolded( false )
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// GetStaticMeshComponent()->OnComponentHit.AddDynamic(this , &AChickenBall::OnBallHit);
	
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->UpdatedComponent = GetStaticMeshComponent();
	ProjectileMovementComponent->OnProjectileBounce.AddDynamic(this, &AChickenBall::OnBallBounce);
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

void AChickenBall::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void AChickenBall::PickUpBall(AChickenRushCharacter* InCharacter)
{
	Character = InCharacter;
	
	GetStaticMeshComponent()->SetSimulatePhysics(false);
	GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetStaticMeshComponent()->SetEnableGravity(false);

	// 禁用ProjectileMovementComponent
	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->Deactivate();
	}

	// 挂在 Mesh的Socket下
	FAttachmentTransformRules Rules( EAttachmentRule::SnapToTarget , false );
	AttachToComponent( InCharacter->GetMesh() , Rules , "headSocket");

	// 挂在 SceneComponent下
	// FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, false);
	// AttachToComponent( Character->SceneSocketComponent, Rules);
	
	bHolded = true;

	UKismetSystemLibrary::PrintString(GetWorld(),TEXT("Ball PickUp Ball ") );
}

void AChickenBall::ThrowBall(const FVector& Direction )
{

	GetStaticMeshComponent()->SetSimulatePhysics(false);
	GetStaticMeshComponent()->SetEnableGravity(false);
	GetStaticMeshComponent()->SetCollisionEnabled( ECollisionEnabled::QueryAndPhysics );

	ProjectileMovementComponent->ProjectileGravityScale = 0.0f;
	ProjectileMovementComponent->InitialSpeed = 1500.0f;
	ProjectileMovementComponent->MaxSpeed = 1500.0f;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->bShouldBounce = true;
	ProjectileMovementComponent->Velocity = Direction * 1500.0f ; 

	FDetachmentTransformRules Rules( EDetachmentRule::KeepWorld , true );
	DetachFromActor( Rules );
	
	ProjectileMovementComponent->Activate();

	bHolded = false;
	
	UKismetSystemLibrary::PrintString(GetWorld(),TEXT("Ball Throw Ball ") );
}

void AChickenBall::OnBallBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	UKismetSystemLibrary::PrintString(GetWorld(),TEXT("On Ball Bounce: ") + ImpactResult.Actor->GetName() );
	AChickenRushCharacter* crc = Cast<AChickenRushCharacter>(ImpactResult.Actor);
	// TODO 除了射球时的持球者外，其余物体都应该有碰撞
	if( crc == NULL || // 不是人
		! ( crc->IsValidLowLevel() && Character->IsValidLowLevel() && crc == Character ) ) // 是人非几
	{
		UKismetSystemLibrary::PrintString(GetWorld(),TEXT("On Ball Bounce: 除了射球时的持球者外，其余物体都应该有碰撞") );
		GetStaticMeshComponent()->SetSimulatePhysics(true);
		GetStaticMeshComponent()->SetEnableGravity(true);
		// ProjectileMovementComponent->Deactivate();
	}
	Character = NULL;
}

// void AChickenBall::OnBallHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
// 	FVector NormalImpulse, const FHitResult& Hit)
// {
// 	UKismetSystemLibrary::PrintString(GetWorld(),TEXT("On Ball Hit: ") + OtherActor->GetName() );
// 	AChickenRushCharacter* crc = Cast<AChickenRushCharacter>(OtherActor);
// 	if( crc == nullptr )
// 	{
// 		GetStaticMeshComponent()->SetSimulatePhysics(true);
// 		GetStaticMeshComponent()->SetEnableGravity(true);
// 		// ProjectileMovementComponent->Deactivate();
// 	}
// }



