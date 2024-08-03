// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "ChickenBall.generated.h"

UCLASS()
class CHICKENRUSH_API AChickenBall : public AStaticMeshActor
{
	GENERATED_BODY()

public:
	AChickenBall();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="My Variables"  )
	bool bHolded;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	UProjectileMovementComponent* ProjectileMovementComponent;


	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PostInitializeComponents() override;

	UFUNCTION()
	void PickUpBall();
	
	UFUNCTION()
	void ThrowBall(const FVector& Direction);

	// UFUNCTION(Server,Reliable)
	// void ServerThrowBall(const FVector& Direction);
	//
	// UFUNCTION(NetMulticast, Reliable)
	// void MulticastThrowBall(const FVector& Direction);

	UFUNCTION()
	void OnBallBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

	UFUNCTION()
	void OnBallHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
};
