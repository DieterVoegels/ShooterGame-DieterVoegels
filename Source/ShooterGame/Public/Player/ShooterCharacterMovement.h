// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * Movement component meant for use with Pawns.
 */

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ShooterCharacterMovement.generated.h"

// Custom movement mode
UENUM(BlueprintType)
enum ECustomMovementMode
{
  SHOOTERMOVE_WallRunning UMETA(DisplayName = "WallRunning")
};

UCLASS()
class UShooterCharacterMovement : public UCharacterMovementComponent
{
  GENERATED_BODY()

  //----------------------------------------------------------------------//
  // Saved Move
  //----------------------------------------------------------------------//

  class FSavedMove_ShooterCharacter : public FSavedMove_Character
  {
  public:
    typedef FSavedMove_Character Super;

    virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const override;
    virtual void Clear() override;
    virtual uint8 GetCompressedFlags() const override;
    virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData) override;
    virtual void PrepMoveFor(ACharacter* Character) override;

    // Teleport
    FVector SavedTeleportDirection;
    uint8 bSavedWantsToTeleport : 1;

    // Wall run
    FVector SavedWallDirection;
    FVector SavedWallRunMovementDirection;
    uint8 bSavedWantsToWallRun : 1;
  };

  //----------------------------------------------------------------------//
  // Network Prediction Data Client Character
  //----------------------------------------------------------------------//

  class FNetworkPredictionData_Client_ShooterCharacter : public FNetworkPredictionData_Client_Character
  {
  public:
    typedef FNetworkPredictionData_Client_Character Super;

    FNetworkPredictionData_Client_ShooterCharacter(const UCharacterMovementComponent& ClientMovement);
    virtual FSavedMovePtr AllocateNewMove() override;
  };

  //----------------------------------------------------------------------//
  // Character Movement
  //----------------------------------------------------------------------//

public:
  UShooterCharacterMovement(const FObjectInitializer& ObjectInitializer);
  virtual void BeginPlay() override;
  virtual float GetMaxSpeed() const override;
  virtual void UpdateFromCompressedFlags(uint8 Flags) override;
  virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;
  void OnMovementUpdated(float DeltaTime, const FVector& OldLocation, const FVector& OldVelocity);
  virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickfunction) override;

  UFUNCTION()
  void OnActorHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit);
  bool IsHoldingJump();

  //////////////////////////////////////////////////////////////////////////
  // Teleport

public:
  // Teleports the player a set distance in the direction they are facing
  UFUNCTION(BlueprintCallable, Category = "Movement|Teleport")
  void Teleport();

protected:
  // The distance to teleport
  UPROPERTY(EditDefaultsOnly, Category = "Movement|Teleport")
  float TeleportDistance;

  // Current teleport state 
  uint8 bWantsToTeleport : 1;

  // Direction to teleport
  FVector TeleportDirection;

  // Update teleport direction
  UFUNCTION(Unreliable, Server, WithValidation)
  void Server_TeleportDirection(const FVector& TeleportDir);

  //////////////////////////////////////////////////////////////////////////
  // Wall Run

public:
  // Initiates wall run
  UFUNCTION(BlueprintCallable, Category = "Movement|Wall Run")
  void StartWallRun(const FVector& Direction);

  // Stops wall run
  UFUNCTION(BlueprintCallable, Category = "Movement|Wall Run")
  void StopWallRun();

protected:

  virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
  virtual void PhysCustom(float deltaTime, int32 Iterations) override;

  // Checks for a wall in a certain direction, returning the hit result
  FHitResult CheckWallProximity(FVector Direction);

  // Maximum distance from the wall to wall run
  UPROPERTY(EditDefaultsOnly, Category = "Movement|Wall Run")
  float WallMaxDistance;

  // Maximum difference between each wall normal, 0 to 1. The Lower the number, the tighter the corners you can wall run.
  UPROPERTY(EditDefaultsOnly, Category = "Movement|Wall Run")
  float WallRunCornerVariance;

  // How fast you wall run
  UPROPERTY(EditDefaultsOnly, Category = "Movement|Wall Run")
  float WallRunSpeed;

  // Gravity when wall running
  UPROPERTY(EditDefaultsOnly, Category = "Movement|Wall Run")
  float WallRunGravity;

  // How much force to launch for a wall jump
  UPROPERTY(EditDefaultsOnly, Category = "Movement|Wall Run")
  float WallJumpForce;

  // player is holding jump
  bool bHoldingJump;

  // The direction the wall is facing
  FVector WallDirection;

  // The direction character is moving
  FVector WallRunMovementDirection;

  // Gravity when not wall running
  float DefaultGravity;

  // Wall direction replication
  UFUNCTION(Unreliable, Server, WithValidation)
  void Server_WallDirection(const FVector& Direction);

  // Movement direction replication
  UFUNCTION(Unreliable, Server, WithValidation)
  void Server_WallRunMovementDirection(const FVector& Direction);

  // Holding jump replication
  UFUNCTION(Unreliable, Server, WithValidation)
  void Server_HoldingJump(bool IsHoldingJump);
};
