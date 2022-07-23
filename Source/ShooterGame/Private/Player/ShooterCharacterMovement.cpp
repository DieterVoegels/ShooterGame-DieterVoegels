// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterGame.h"
#include "Player/ShooterCharacter.h"
#include "Player/ShooterCharacterMovement.h"

//----------------------------------------------------------------------//
// Saved Move
//----------------------------------------------------------------------//

void UShooterCharacterMovement::FSavedMove_ShooterCharacter::Clear()
{
	Super::Clear();

	// Teleport
	bSavedWantsToTeleport = false;
	SavedTeleportDirection = FVector::ZeroVector;

	// Wall Run
	SavedWallDirection = FVector::ZeroVector;
	SavedWallRunMovementDirection = FVector::ZeroVector;
}

uint8 UShooterCharacterMovement::FSavedMove_ShooterCharacter::GetCompressedFlags() const
{
	uint8 Flags = Super::GetCompressedFlags();

	// Teleport
	if (bSavedWantsToTeleport)
	{
		Flags |= FLAG_Custom_0;
	}

	return Flags;
}

bool UShooterCharacterMovement::FSavedMove_ShooterCharacter::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const
{
	return Super::CanCombineWith(NewMove, Character, MaxDelta);
}

void UShooterCharacterMovement::FSavedMove_ShooterCharacter::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);

	UShooterCharacterMovement* CharacterMovement = Cast<UShooterCharacterMovement>(Character->GetCharacterMovement());
	if (CharacterMovement)
	{
		// Teleport
		bSavedWantsToTeleport = CharacterMovement->bWantsToTeleport;
		SavedTeleportDirection = CharacterMovement->TeleportDirection;
		
		// Wall Direction
		SavedWallDirection = CharacterMovement->WallDirection;
		SavedWallRunMovementDirection = CharacterMovement->WallRunMovementDirection;
	}
}

void UShooterCharacterMovement::FSavedMove_ShooterCharacter::PrepMoveFor(class ACharacter* Character)
{
	Super::PrepMoveFor(Character);

	UShooterCharacterMovement* CharacterMovement = Cast<UShooterCharacterMovement>(Character->GetCharacterMovement());
	if (CharacterMovement)
	{
		// Teleport
		CharacterMovement->TeleportDirection = SavedTeleportDirection;

		// Wall Run
		CharacterMovement->WallDirection = SavedWallDirection;
		CharacterMovement->WallRunMovementDirection = SavedWallRunMovementDirection;
	}
}

//----------------------------------------------------------------------//
// Network Prediction Data Client Character
//----------------------------------------------------------------------//

UShooterCharacterMovement::FNetworkPredictionData_Client_ShooterCharacter::FNetworkPredictionData_Client_ShooterCharacter(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{
	
}

FSavedMovePtr UShooterCharacterMovement::FNetworkPredictionData_Client_ShooterCharacter::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_ShooterCharacter);
}

//----------------------------------------------------------------------//
// Character Movement
//----------------------------------------------------------------------//

UShooterCharacterMovement::UShooterCharacterMovement(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultGravity = GravityScale;
}

void UShooterCharacterMovement::BeginPlay()
{
	Super::BeginPlay();

	if (PawnOwner)
	{
		PawnOwner->OnActorHit.AddDynamic(this, &UShooterCharacterMovement::OnActorHit);
	}
}

void UShooterCharacterMovement::OnActorHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit)
{
	// If we are jumping, check for wall running
	if (bHoldingJump)
	{
		if (MovementMode == MOVE_Custom && CustomMovementMode == SHOOTERMOVE_WallRunning)
		{
			return;
		}

		// Calculate how vertical the normal vector is
		float NormalDirection = FVector::DotProduct(FVector(0.0f, 0.0f, 1.0f), Hit.ImpactNormal);

		// If collided with a horizontal surface, we are wall running
		if (abs(NormalDirection) < 0.1f)
		{
			StartWallRun(Hit.ImpactPoint - GetActorLocation());
		}
	}
}



//////////////////////////////////////////////////////////////////////////
// Client Prediction

void UShooterCharacterMovement::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	// Teleport
	bWantsToTeleport = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}

class FNetworkPredictionData_Client* UShooterCharacterMovement::GetPredictionData_Client() const
{
	check(PawnOwner != NULL);
	check(PawnOwner->GetLocalRole() < ROLE_Authority);

	if (!ClientPredictionData)
	{
		UShooterCharacterMovement* MutableThis = const_cast<UShooterCharacterMovement*>(this);

		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_ShooterCharacter(*this);
	}

	return ClientPredictionData;
}

//////////////////////////////////////////////////////////////////////////
// Movement Updates

void UShooterCharacterMovement::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (PawnOwner && PawnOwner->IsLocallyControlled())
	{
		bHoldingJump = IsHoldingJump();
		Server_HoldingJump(bHoldingJump);
	}

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UShooterCharacterMovement::OnMovementUpdated(float DeltaTime, const FVector& OldLocation, const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaTime, OldLocation, OldVelocity);

	if (!CharacterOwner)
	{
		return;
	}

	// Teleport
	if (bWantsToTeleport)
	{
		bWantsToTeleport = false;

		TeleportDirection.Z = 0.0f;
		FVector NewLocation = CharacterOwner->GetActorLocation() + (TeleportDirection * TeleportDistance);

		PawnOwner->SetActorLocation(NewLocation, true);
	}
}

void UShooterCharacterMovement::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	// changed to wall running
	if (MovementMode == MOVE_Custom)
	{
		if (CustomMovementMode == ECustomMovementMode::SHOOTERMOVE_WallRunning)
		{
			GravityScale = WallRunGravity;
			Velocity.Z = 0.0f;
		}
	}

	// changed from wall running
	if (PreviousMovementMode == MOVE_Custom)
	{
		if (PreviousCustomMode == ECustomMovementMode::SHOOTERMOVE_WallRunning)
		{
			GravityScale = DefaultGravity;
		}
	}
}

void UShooterCharacterMovement::PhysCustom(float deltaTime, int32 Iterations)
{
	if (PawnOwner && PawnOwner->GetLocalRole() == ROLE_SimulatedProxy)
	{
		return;
	}

	if (!CharacterOwner)
	{
		return;
	}

	// Wall run
	if (MovementMode == MOVE_Custom && CustomMovementMode == ECustomMovementMode::SHOOTERMOVE_WallRunning)
	{
		// If character lets go of the jump key, then jump off of the wall
		if (!bHoldingJump)
		{
			FVector JumpDirection = WallDirection * -1.0f + CharacterOwner->GetActorUpVector();
			JumpDirection.Normalize();

			Launch(JumpDirection * WallJumpForce + Velocity);

			StopWallRun();
		}

		// Update velocity direction to be tangent to the wall
		// If the wall bends to much, such as a corner, then stop wall run
		FHitResult WallHit = CheckWallProximity(WallDirection);

		if (WallHit.bBlockingHit && abs(FVector::DotProduct(WallDirection, WallHit.Normal)) > WallRunCornerVariance)
		{
			WallDirection = WallHit.Normal * -1.0f;
			FVector VelocityDirection = FVector::VectorPlaneProject(Velocity, WallDirection);
			Velocity = VelocityDirection.GetSafeNormal() * 1000.0f;

			const FVector AdjustedVelocity = Velocity * deltaTime;
			FHitResult Hit(1.0f);
			SafeMoveUpdatedComponent(AdjustedVelocity, UpdatedComponent->GetComponentQuat(), false, Hit);
		}
		else
		{
			StopWallRun();
		}
	}

	Super::PhysCustom(deltaTime, Iterations);
}

bool UShooterCharacterMovement::IsHoldingJump()
{
	if (PawnOwner && PawnOwner->IsLocallyControlled())
	{
		TArray<FInputActionKeyMapping> JumpKeyMappings;
		UInputSettings::GetInputSettings()->GetActionMappingByName("Jump", JumpKeyMappings);
		for (FInputActionKeyMapping& JumpKeyMapping : JumpKeyMappings)
		{
			AShooterPlayerController* controller = PawnOwner->GetController<AShooterPlayerController>();
			if (controller && controller->IsInputKeyDown(JumpKeyMapping.Key))
			{
				return true;
			}
		}
	}

	return false;
}

float UShooterCharacterMovement::GetMaxSpeed() const
{
	float MaxSpeed = Super::GetMaxSpeed();

	const AShooterCharacter* ShooterCharacterOwner = Cast<AShooterCharacter>(PawnOwner);
	if (ShooterCharacterOwner)
	{
		if (ShooterCharacterOwner->IsTargeting())
		{
			MaxSpeed *= ShooterCharacterOwner->GetTargetingSpeedModifier();
		}
		if (ShooterCharacterOwner->IsRunning())
		{
			MaxSpeed *= ShooterCharacterOwner->GetRunningSpeedModifier();
		}
	}

	return MaxSpeed;
}

//////////////////////////////////////////////////////////////////////////
// Teleport

void UShooterCharacterMovement::Teleport()
{
	if (PawnOwner && PawnOwner->IsLocallyControlled())
	{
		TeleportDirection = CharacterOwner->GetActorForwardVector().GetSafeNormal();
		Server_TeleportDirection(TeleportDirection);
	}

	bWantsToTeleport = true;
}

bool UShooterCharacterMovement::Server_TeleportDirection_Validate(const FVector& TeleportDir)
{
	return true;
}

void UShooterCharacterMovement::Server_TeleportDirection_Implementation(const FVector& TeleportDir)
{
	TeleportDirection = TeleportDir;
}

//////////////////////////////////////////////////////////////////////////
// Wall Run

void UShooterCharacterMovement::StartWallRun(const FVector& Direction)
{
	if (PawnOwner && PawnOwner->IsLocallyControlled())
	{
		WallDirection = Direction.GetSafeNormal();
		WallRunMovementDirection = FVector::VectorPlaneProject(Velocity, WallDirection).GetSafeNormal();

		Server_WallDirection(WallDirection);
		Server_WallRunMovementDirection(WallRunMovementDirection);
	}

	SetMovementMode(EMovementMode::MOVE_Custom, ECustomMovementMode::SHOOTERMOVE_WallRunning);
}

void UShooterCharacterMovement::StopWallRun()
{
	SetMovementMode(EMovementMode::MOVE_Falling);
}

FHitResult UShooterCharacterMovement::CheckWallProximity(FVector Direction)
{
	FHitResult HitOut(ForceInit);

	if (!CharacterOwner)
	{
		return HitOut;
	}

	FVector EndPoint = GetActorLocation() + (Direction * (CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleRadius() + WallMaxDistance));

	// Trace parameters
	FCollisionQueryParams TraceParams = FCollisionQueryParams(FName("WallTraceTag"), true, PawnOwner);
	TraceParams.bTraceComplex = true;
	TraceParams.bReturnPhysicalMaterial = false;
	TraceParams.bFindInitialOverlaps = true;

	// Line trace
	GetWorld()->LineTraceSingleByChannel(
		HitOut,
		GetActorLocation(),
		EndPoint,
		ECollisionChannel::ECC_Visibility,
		TraceParams
	);

	return HitOut;
}

bool UShooterCharacterMovement::Server_WallDirection_Validate(const FVector& Direction)
{
	return true;
}

void UShooterCharacterMovement::Server_WallDirection_Implementation(const FVector& Direction)
{
	WallDirection = Direction;
}

bool UShooterCharacterMovement::Server_WallRunMovementDirection_Validate(const FVector& Direction)
{
	return true;
}

void UShooterCharacterMovement::Server_WallRunMovementDirection_Implementation(const FVector& Direction)
{
	WallRunMovementDirection = Direction;
}

bool UShooterCharacterMovement::Server_HoldingJump_Validate(bool IsHoldingJump)
{
	return true;
}

void UShooterCharacterMovement::Server_HoldingJump_Implementation(bool IsHoldingJump)
{
	bHoldingJump = IsHoldingJump;
}
