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
}

void UShooterCharacterMovement::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

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

void UShooterCharacterMovement::FSavedMove_ShooterCharacter::Clear()
{
	Super::Clear();

	//Teleport
	bSavedWantsToTeleport = false;
	SavedTeleportDirection = FVector::ZeroVector;
}

uint8 UShooterCharacterMovement::FSavedMove_ShooterCharacter::GetCompressedFlags() const
{
	uint8 Flags = Super::GetCompressedFlags();

	//Teleport
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
		//Teleport
		bSavedWantsToTeleport = CharacterMovement->bWantsToTeleport;
		SavedTeleportDirection = CharacterMovement->TeleportDirection;
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