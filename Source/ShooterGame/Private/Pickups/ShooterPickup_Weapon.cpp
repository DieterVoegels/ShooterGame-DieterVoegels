#include "ShooterGame.h"
#include "Pickups/ShooterPickup_Weapon.h"
#include "Weapons/ShooterWeapon.h"
#include "OnlineSubsystemUtils.h"

AShooterPickup_Weapon::AShooterPickup_Weapon(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	Mesh = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("WeaponPickupMesh"));
	Mesh->SetupAttachment(RootComponent);

	AmmoQty = 0;
}

bool AShooterPickup_Weapon::IsForWeapon(UClass* WeaponClass)
{
	return WeaponType->IsChildOf(WeaponClass);
}

bool AShooterPickup_Weapon::CanBePickedUp(AShooterCharacter* TestPawn) const
{
	AShooterWeapon* TestWeapon = (TestPawn ? TestPawn->FindWeapon(WeaponType) : NULL);
	if (bIsActive)
	{
		if (TestWeapon)
		{
			return TestWeapon->GetCurrentAmmo() < TestWeapon->GetMaxAmmo();
		}
		else
		{
			return true;
		}
	}

	return false;
}

void AShooterPickup_Weapon::SetMesh(USkeletalMeshComponent* NewMesh)
{
	Mesh->SetSkeletalMesh(NewMesh->SkeletalMesh);
}

void AShooterPickup_Weapon::GivePickupTo(class AShooterCharacter* Pawn)
{
	AShooterWeapon* Weapon = (Pawn ? Pawn->FindWeapon(WeaponType) : NULL);
	if (Weapon)
	{
		// Give the other weapon the remaining ammo of this weapon
		Weapon->GiveAmmo(AmmoQty);

		// Fire event for collected weapon
		if (Pawn)
		{
			const UWorld* World = GetWorld();
			const IOnlineEventsPtr Events = Online::GetEventsInterface(World);
			const IOnlineIdentityPtr Identity = Online::GetIdentityInterface(World);

			if (Events.IsValid() && Identity.IsValid())
			{
				AShooterPlayerController* PC = Cast<AShooterPlayerController>(Pawn->Controller);
				if (PC)
				{
					ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(PC->Player);

					if (LocalPlayer)
					{
						const int32 UserIndex = LocalPlayer->GetControllerId();
						TSharedPtr<const FUniqueNetId> UniqueID = Identity->GetUniquePlayerId(UserIndex);
						if (UniqueID.IsValid())
						{
							FVector Location = Pawn->GetActorLocation();

							FOnlineEventParms Params;

							Params.Add(TEXT("SectionId"), FVariantData((int32)0)); // unused
							Params.Add(TEXT("GameplayModeId"), FVariantData((int32)1));
							Params.Add(TEXT("DifficultyLevelId"), FVariantData((int32)0)); // unused

							Params.Add(TEXT("ItemId"), FVariantData((int32)Weapon->GetAmmoType() + 1));
							Params.Add(TEXT("AcquisitionMethodId"), FVariantData((int32)0)); // unused
							Params.Add(TEXT("LocationX"), FVariantData(Location.X));
							Params.Add(TEXT("LocationY"), FVariantData(Location.Y));
							Params.Add(TEXT("LocationZ"), FVariantData(Location.Z));
							Params.Add(TEXT("ItemQty"), FVariantData((int32)AmmoQty));

							Events->TriggerEvent(*UniqueID, TEXT("CollectPowerup"), Params);
						}
					}
				}
			}
		}
	}
}

void AShooterPickup_Weapon::OnPickedUp()
{
	Super::OnPickedUp();
	Destroy();
}