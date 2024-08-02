// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChickenRushCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"

//////////////////////////////////////////////////////////////////////////
// AChickenRushCharacter

AChickenRushCharacter::AChickenRushCharacter():
	bHoldingBall(false)
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

void AChickenRushCharacter::BeginPlay()
{
	Super::BeginPlay();
	TArray<AActor*> AllActors;
	UGameplayStatics::GetAllActorsOfClass( GetWorld() , AChickenBall::StaticClass() , AllActors );
	// 先假设小球始终唯一。
	for( AActor* actor : AllActors )
	{
		AChickenBall* ChickenBall = CastChecked<AChickenBall>(actor);
		Ball = ChickenBall;
		UKismetSystemLibrary::PrintString(GetWorld(),"BeginPlay Set Ball" );

		break;
	}
	
}

//////////////////////////////////////////////////////////////////////////
// Input

void AChickenRushCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &AChickenRushCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AChickenRushCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AChickenRushCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AChickenRushCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AChickenRushCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AChickenRushCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AChickenRushCharacter::OnResetVR);

	PlayerInputComponent->BindAction("Pick", IE_Pressed, this, &AChickenRushCharacter::OnPick);
	PlayerInputComponent->BindAction("Throw", IE_Pressed, this, &AChickenRushCharacter::OnThrow);
	
}

void AChickenRushCharacter::OnPick()
{
	UKismetSystemLibrary::PrintString(GetWorld(),"OnPick" );

	// if( ! HasAuthority() )
	{
		ServerPickUpBall();
	}
}

void AChickenRushCharacter::OnThrow()
{
	UKismetSystemLibrary::PrintString(GetWorld(),"OnThrow" );

	// if( ! HasAuthority() )
	{
		ServerThrowBall();
	}
}

void AChickenRushCharacter::PickUpBall()
{
	Ball->GetStaticMeshComponent()->SetSimulatePhysics(false);
	Ball->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ball->GetStaticMeshComponent()->SetEnableGravity(false);
	Ball->SetReplicates(false);
	Ball->SetReplicateMovement(false);
	FAttachmentTransformRules Rules( EAttachmentRule::SnapToTarget , false );
	Ball->AttachToComponent( GetMesh() , Rules , "headSocket");
	bHoldingBall = true;
	Ball->bHolded = true;
	UKismetSystemLibrary::PrintString(GetWorld(),"Pick Up Ball" );
	
}

void AChickenRushCharacter::ThrowBall()
{
	Ball->GetStaticMeshComponent()->SetSimulatePhysics(true);
	Ball->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Ball->GetStaticMeshComponent()->SetEnableGravity(true);
	Ball->SetReplicates(true);
	Ball->SetReplicateMovement(true);
	FDetachmentTransformRules Rules( EDetachmentRule::KeepWorld , true );
	Ball->DetachFromActor( Rules );
	bHoldingBall = false;
	Ball->bHolded = false;
	FVector Pulse = GetMesh()->GetRightVector() * 1500.0f;
	Ball->GetStaticMeshComponent()->AddImpulse( Pulse , NAME_None , true );
	UKismetSystemLibrary::PrintString(GetWorld(),"Throw Ball" );
}

void AChickenRushCharacter::ServerThrowBall_Implementation()
{
	UKismetSystemLibrary::PrintString(GetWorld(),"Server Throw Ball" );
	if( bHoldingBall == true && Ball->IsValidLowLevel() && Ball->bHolded == true )
	{
		UKismetSystemLibrary::PrintString(GetWorld(),"Server Meets Holding" );
		ThrowBall();
		MulticastThrowBall();
	}
}

bool AChickenRushCharacter::ServerThrowBall_Validate()
{
	return true;
}

void AChickenRushCharacter::MulticastThrowBall_Implementation()
{
	if( ! HasAuthority() )
	{
		ThrowBall();
	}
}

void AChickenRushCharacter::ServerPickUpBall_Implementation()
{
	UKismetSystemLibrary::PrintString(GetWorld(),"Server PickUp Ball" );
	// 未持球，场上有球。
	if( bHoldingBall == false && Ball->IsValidLowLevel() && Ball->bHolded == false )
	{
		UKismetSystemLibrary::PrintString(GetWorld(),"Server Meets Not Holding" );

		// 距离
		FVector BallPos = Ball->GetActorLocation();
		FVector SelfPos = GetActorLocation();
		if( (BallPos - SelfPos).Size() < 400.0f )
		{
			PickUpBall();
			MulticastPickUpBall();
		}
	}
}

bool AChickenRushCharacter::ServerPickUpBall_Validate()
{
	return true;
}

void AChickenRushCharacter::MulticastPickUpBall_Implementation()
{
	if( ! HasAuthority() )
	{
		PickUpBall();
	}
}


void AChickenRushCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// DOREPLIFETIME( AChickenRushCharacter , Ball );
	// DOREPLIFETIME( AChickenRushCharacter , bHoldingBall );
}

void AChickenRushCharacter::OnResetVR()
{
	// If ChickenRush is added to a project via 'Add Feature' in the Unreal Editor the dependency on HeadMountedDisplay in ChickenRush.Build.cs is not automatically propagated
	// and a linker error will result.
	// You will need to either:
	//		Add "HeadMountedDisplay" to [YourProject].Build.cs PublicDependencyModuleNames in order to build successfully (appropriate if supporting VR).
	// or:
	//		Comment or delete the call to ResetOrientationAndPosition below (appropriate if not supporting VR)
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AChickenRushCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void AChickenRushCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void AChickenRushCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AChickenRushCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AChickenRushCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AChickenRushCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}