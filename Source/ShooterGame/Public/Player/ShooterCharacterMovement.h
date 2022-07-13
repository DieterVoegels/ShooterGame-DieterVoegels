// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * Movement component meant for use with Pawns.
 */

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ShooterCharacterMovement.generated.h"

UCLASS()
class UShooterCharacterMovement : public UCharacterMovementComponent
{
  GENERATED_BODY()

  UShooterCharacterMovement(const FObjectInitializer& ObjectInitializer);
	virtual float GetMaxSpeed() const override;

  class FSavedMove_ShooterCharacter : public FSavedMove_Character
  {
  public:
    typedef FSavedMove_Character Super;

    virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const override;
    virtual void Clear() override;
    virtual uint8 GetCompressedFlags() const override;
    virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData) override;
    virtual void PrepMoveFor(ACharacter* Character) override;

    //Teleport
    FVector SavedTeleportDirection;
    uint8 bSavedWantsToTeleport : 1;
  };

  class FNetworkPredictionData_Client_ShooterCharacter : public FNetworkPredictionData_Client_Character
  {
  public:
    typedef FNetworkPredictionData_Client_Character Super;

    FNetworkPredictionData_Client_ShooterCharacter(const UCharacterMovementComponent& ClientMovement);
    virtual FSavedMovePtr AllocateNewMove() override;
  };

public:
  virtual void UpdateFromCompressedFlags(uint8 Flags) override;
  virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;
  void OnMovementUpdated(float DeltaTime, const FVector& OldLocation, const FVector& OldVelocity);
  
  FVector TeleportDirection;
  uint8 bWantsToTeleport : 1;

  UPROPERTY(EditAnywhere, Category = "Teleport")
    float TeleportDistance;

  UFUNCTION(Unreliable, Server, WithValidation)
    void Server_TeleportDirection(const FVector& TeleportDir);

  UFUNCTION(BlueprintCallable, Category = "Teleport")
    void Teleport();
};
