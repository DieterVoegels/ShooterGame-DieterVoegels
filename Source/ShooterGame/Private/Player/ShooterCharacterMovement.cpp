// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterGame.h"
#include "Player/ShooterCharacter.h"
#include "Player/ShooterCharacterMovement.h"

//----------------------------------------------------------------------//
// UPawnMovementComponent
//----------------------------------------------------------------------//
UShooterCharacterMovement::UShooterCharacterMovement(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	TeleportDistance = 1000.0f;
	DefaultGravity = GravityScale;
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

void UShooterCharacterMovement::OnMovementUpdated(float DeltaTime, const FVector& OldLocation, const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaTime, OldLocation, OldVelocity);

	if (!CharacterOwner)
	{
		return;
	}

	//Teleport
	if (bWantsToTeleport)
	{
		bWantsToTeleport = false;

		TeleportDirection.Normalize();

		//Don't teleport into the air
		TeleportDirection.Z = 0.0f;
		FVector NewLocation = CharacterOwner->GetActorLocation() + TeleportDirection * TeleportDistance;

		//Set player location
		CharacterOwner->SetActorLocation(NewLocation, false, nullptr, ETeleportType::None);
	}

	//Wall run
	if (bWantsToWallRun)
	{
		// If character lets go of Jump, then jump from the wall
		if (static_cast<AShooterCharacter*>(CharacterOwner)->bHoldingJump == false)
		{
			GravityScale = 1.0f;
			bWantsToWallRun = false;

			FVector JumpDirection = WallDirection * -1.0f + CharacterOwner->GetActorUpVector();
			JumpDirection.Normalize();

			Launch(JumpDirection * WallJumpForce + Velocity);
		}

		FHitResult WallHit = CheckWallProximity(WallDirection);

		// Update velocity direction to be tangent to the wall, unless it is a corner
		if (WallHit.bBlockingHit && abs(FVector::DotProduct(WallDirection, WallHit.Normal)) > WallRunCornerVariance)
		{
			WallDirection = WallHit.Normal * -1.0f;
			FVector VelocityDirection = FVector::VectorPlaneProject(Velocity, WallDirection);
			VelocityDirection.Normalize();
			Velocity = VelocityDirection * 1000.0f;
		}
		else
		{
			GravityScale = 1.0f;
			bWantsToWallRun = false;
		}
	}
}

void UShooterCharacterMovement::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	//Teleport
	bWantsToTeleport = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;

	//Wall Run
	bWantsToWallRun = (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
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

void UShooterCharacterMovement::FSavedMove_ShooterCharacter::Clear()
{
	Super::Clear();

	//Teleport
	bSavedWantsToTeleport = false;
	SavedTeleportDirection = FVector::ZeroVector;

	//Wall Run
	bSavedWantsToWallRun = false;
	SavedWallDirection = FVector::ZeroVector;
	SavedWallRunMovementDirection = FVector::ZeroVector;
}

uint8 UShooterCharacterMovement::FSavedMove_ShooterCharacter::GetCompressedFlags() const
{
	uint8 Flags = Super::GetCompressedFlags();

	//Teleport
	if (bSavedWantsToTeleport)
	{
		Flags |= FLAG_Custom_0;
	}

	//Wall Run
	if (bSavedWantsToWallRun)
	{
		Flags |= FLAG_Custom_1;
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
		//Teleport
		bSavedWantsToTeleport = CharacterMovement->bWantsToTeleport;
		SavedTeleportDirection = CharacterMovement->TeleportDirection;
		
		//Wall Direction
		bSavedWantsToWallRun = CharacterMovement->bWantsToWallRun;
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
		//Teleport
		CharacterMovement->TeleportDirection = SavedTeleportDirection;

		//Wall Run
		CharacterMovement->WallDirection = SavedWallDirection;
		CharacterMovement->WallRunMovementDirection = SavedWallRunMovementDirection;
	}
}

UShooterCharacterMovement::FNetworkPredictionData_Client_ShooterCharacter::FNetworkPredictionData_Client_ShooterCharacter(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{
	
}

FSavedMovePtr UShooterCharacterMovement::FNetworkPredictionData_Client_ShooterCharacter::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_ShooterCharacter);
}

bool UShooterCharacterMovement::Server_TeleportDirection_Validate(const FVector& TeleportDir)
{
	return true;
}

void UShooterCharacterMovement::Server_TeleportDirection_Implementation(const FVector& TeleportDir)
{
	TeleportDirection = TeleportDir;
}

void UShooterCharacterMovement::Teleport()
{
	if (PawnOwner->IsLocallyControlled())
	{
		TeleportDirection = CharacterOwner->GetActorForwardVector();
		Server_TeleportDirection(TeleportDirection);
	}

	bWantsToTeleport = true;
}

FHitResult UShooterCharacterMovement::CheckWallProximity(FVector Direction)
{
	FHitResult HitOut(ForceInit);
	FVector EndPoint = GetActorLocation() + (Direction * (CharacterOwner->GetCapsuleComponent()->GetUnscaledCapsuleRadius() + WallMaxDistance));

	//Trace parameters
	FCollisionQueryParams TraceParams = FCollisionQueryParams(FName("WallTraceTag"), true, PawnOwner);
	TraceParams.bTraceComplex = true;
	TraceParams.bReturnPhysicalMaterial = false;
	TraceParams.bFindInitialOverlaps = true;

	//Line trace
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

void UShooterCharacterMovement::WallRun(const FVector& Direction)
{
	//start wall run by initializing variables
	if (PawnOwner->IsLocallyControlled())
	{
		WallDirection = Direction;
		WallDirection.Normalize();

		WallRunMovementDirection = FVector::VectorPlaneProject(Velocity, WallDirection);
		WallRunMovementDirection.Normalize();

		GravityScale = WallRunGravity;
		Velocity.Z = 0.0f;

		Server_WallDirection(WallDirection);
		Server_WallRunMovementDirection(WallRunMovementDirection);
	}

	bWantsToWallRun = true;
}