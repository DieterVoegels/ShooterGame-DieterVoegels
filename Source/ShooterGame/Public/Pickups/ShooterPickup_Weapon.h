#pragma once

#include "ShooterPickup.h"
#include "ShooterPickup_Weapon.generated.h"

class AShooterCharacter;
class AShooterWeapon;

// A pickup object that replenishes ammunition for a weapon
UCLASS(Blueprintable)
class AShooterPickup_Weapon : public AShooterPickup
{
	GENERATED_UCLASS_BODY()

	/** check if pawn can use this pickup */
	virtual bool CanBePickedUp(AShooterCharacter* TestPawn) const override;

	bool IsForWeapon(UClass* WeaponClass);

public:

	void SetMesh(USkeletalMeshComponent* NewMesh);

	/** which weapon is this pickup? */
	TSubclassOf<AShooterWeapon> WeaponType;

	/** how much ammo does it give? */
	int32 AmmoQty;

protected:
	/** give pickup */
	virtual void GivePickupTo(AShooterCharacter* Pawn) override;

	/** destroys weapon on pickup */
	virtual void OnPickedUp() override;

private:
	/** weapon mesh*/
	USkeletalMeshComponent* Mesh;
};
