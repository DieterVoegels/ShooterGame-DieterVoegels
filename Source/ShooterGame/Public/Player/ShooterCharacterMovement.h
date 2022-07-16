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

  //////////////////////////////////////////////////////////////////////////
  // Saves the character movement into a list
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

    FVector SavedWallDirection;
    FVector SavedWallRunMovementDirection;
    uint8 bSavedWantsToWallRun : 1;
  };

  //////////////////////////////////////////////////////////////////////////
  // Network Prediction
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
  
  //////////////////////////////////////////////////////////////////////////
  // Teleport

  //Direction to teleport
  FVector TeleportDirection;

  //Current teleport state 
  uint8 bWantsToTeleport : 1;

  //The distance to teleport
  UPROPERTY(EditAnywhere, Category = "Movement|Teleport")
    float TeleportDistance;

  //Update teleport direction
  UFUNCTION(Unreliable, Server, WithValidation)
    void Server_TeleportDirection(const FVector& TeleportDir);

  //Teleports the player a set distance in the direction they are facing
  UFUNCTION(BlueprintCallable, Category = "Movement|Teleport")
    void Teleport();

  //////////////////////////////////////////////////////////////////////////
  // Wall Run

  //Checks for a wall in a certain direction and distance away, returning the hit result
  FHitResult CheckWallProximity(FVector Direction);

  uint8 bWantsToWallRun;
  FVector WallDirection;
  FVector WallRunMovementDirection;

  //Maximum distance from the wall to wall run
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Wall Run")
    float WallMaxDistance;

  //Maximum difference between each wall normal, 0 to 1. The Lower the number, the tighter the corners you can wall run.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Wall Run")
    float WallRunCornerVariance;

  //How fast you wall run
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Wall Run")
    float WallRunSpeed;

  //Gravity when wall running
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Wall Run")
    float WallRunGravity;

  //Gravity when not wall running
  float DefaultGravity;

  //How much force to launch for a wall jump
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Wall Run")
    float WallJumpForce;

  // Wall direction replication
  UFUNCTION(Unreliable, Server, WithValidation)
    void Server_WallDirection(const FVector& Direction);

  // Movement direction replication
  UFUNCTION(Unreliable, Server, WithValidation)
    void Server_WallRunMovementDirection(const FVector& Direction);

  // Initiates wall run
  UFUNCTION(BlueprintCallable, Category = "Movement|Wall Run")
  void WallRun(const FVector& Direction);
};
