// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChickenRushCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/ArrowComponent.h"
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

	// Socket Component for Chicken Ball
	SceneSocketComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneSocketComponent"));
	SceneSocketComponent->SetupAttachment(RootComponent);
	SceneSocketComponent->SetRelativeLocation(FVector(100.0f, 0.0f, 50.0f)); // 例如设置在前方100单位，Z轴50单位处
	SceneSocketComponent->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

void AChickenRushCharacter::BeginPlay()
{
	Super::BeginPlay();
	TArray<AActor*> AllActors;
	UGameplayStatics::GetAllActorsOfClass( GetWorld() , AChickenBall::StaticClass() , AllActors );
	// 先假设小球始终唯一。 TODO 建立小球管理系统
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
	Ball->PickUpBall( this );

	bHoldingBall = true;
	UKismetSystemLibrary::PrintString(GetWorld(),"Pick Up Ball" );
	
}

void AChickenRushCharacter::ThrowBall()
{
	UKismetSystemLibrary::PrintString(GetWorld(),"Character Throw Ball" );
	bHoldingBall = false;
	
	// TODO 对小球的调用改成事件
	FVector LaunchDir = GetMesh()->GetRightVector();
	FVector HorizontalDir( LaunchDir.X , LaunchDir.Y , 0 );
	Ball->ThrowBall( HorizontalDir );
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
	// AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AChickenRushCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	// AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AChickenRushCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		Direction = FVector(1,0,0);
		
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
		FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		Direction = FVector(0,1,0);
		
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}
