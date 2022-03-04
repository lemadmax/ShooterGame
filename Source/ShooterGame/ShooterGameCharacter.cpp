// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterGameCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Door.h"
#include "DrawDebugHelpers.h"


#define INTERACTABLE_CHANNEL ECC_GameTraceChannel1

//////////////////////////////////////////////////////////////////////////
// AShooterGameCharacter

AShooterGameCharacter::AShooterGameCharacter()
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
	USkeletalMeshComponent* mesh = GetMesh();
	UCapsuleComponent* capsule = GetCapsuleComponent();
	mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	//capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	capsule->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
	capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
}

//////////////////////////////////////////////////////////////////////////
// Input

void AShooterGameCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Run", IE_Pressed, this, &AShooterGameCharacter::Run);
	PlayerInputComponent->BindAction("StopRun", IE_Released, this, &AShooterGameCharacter::StopRun);
	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AShooterGameCharacter::ClientInteract);

	PlayerInputComponent->BindAxis("MoveForward", this, &AShooterGameCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AShooterGameCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AShooterGameCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AShooterGameCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AShooterGameCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AShooterGameCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AShooterGameCharacter::OnResetVR);
}


void AShooterGameCharacter::OnResetVR()
{
	// If ShooterGame is added to a project via 'Add Feature' in the Unreal Editor the dependency on HeadMountedDisplay in ShooterGame.Build.cs is not automatically propagated
	// and a linker error will result.
	// You will need to either:
	//		Add "HeadMountedDisplay" to [YourProject].Build.cs PublicDependencyModuleNames in order to build successfully (appropriate if supporting VR).
	// or:
	//		Comment or delete the call to ResetOrientationAndPosition below (appropriate if not supporting VR)
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AShooterGameCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void AShooterGameCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void AShooterGameCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AShooterGameCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AShooterGameCharacter::MoveForward(float Value)
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

void AShooterGameCharacter::MoveRight(float Value)
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

void AShooterGameCharacter::Run()
{
	GetCharacterMovement()->MaxWalkSpeed = 1200;
	if(GetLocalRole() < ROLE_Authority)
		ServerSetRunning(true);
}

void AShooterGameCharacter::StopRun()
{
	GetCharacterMovement()->MaxWalkSpeed = 600;
	if(GetLocalRole() < ROLE_Authority)
		ServerSetRunning(false);
}

bool AShooterGameCharacter::ServerSetRunning_Validate(bool isRunning)
{
	return true;
}

void AShooterGameCharacter::ServerSetRunning_Implementation(bool isRunning)
{
	if (isRunning)
	{
		Run();
	}
	else
	{
		StopRun();
	}
}

void AShooterGameCharacter::ClientInteract()
{
	if (GetLocalRole() < ROLE_Authority)
	{
		ServerInteract();
	}
	else 
	{
		Interact();
	}
}

void AShooterGameCharacter::Interact()
{
	FVector start_location = GetActorLocation();
	FVector end_location = start_location + GetActorForwardVector() * 200.f;
	FHitResult hitinfo(ForceInit);
	bool hitted = GetWorld()->LineTraceSingleByChannel(hitinfo, start_location, end_location, ECollisionChannel::ECC_GameTraceChannel1);
	DrawDebugLine(GetWorld(), start_location, end_location, FColor::Green, false, 3.f, false, 2.f);
	if (hitted)
	{
		AActor *hit_actor = hitinfo.GetActor();
		DrawDebugSphere(GetWorld(), hit_actor->GetActorLocation(), 20.f, 32, FColor::Red, false, 3.f, false, 2.f);
		ADoor *hit_obj = Cast<ADoor>(hit_actor);
		if (hit_obj)
		{
			hit_obj->Interacted(Controller->GetPawn());
			UE_LOG(LogTemp, Warning, TEXT("ray hit"));
		}
	}
}


bool AShooterGameCharacter::ServerInteract_Validate()
{
	return true;
}

void AShooterGameCharacter::ServerInteract_Implementation()
{
	Interact();
}