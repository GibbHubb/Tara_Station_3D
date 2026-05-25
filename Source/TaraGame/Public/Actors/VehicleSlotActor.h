#pragma once

#include "CoreMinimal.h"
#include "Actors/InteractableActor.h"
#include "Entities/Vehicle.h"
#include "VehicleSlotActor.generated.h"

class UStaticMeshComponent;

// One parking-bay actor inside the Shed (TS-3D-PHASE2-shed-cottage plan §6).
// Holds a target EVehicleType + a vehicle-body StaticMesh that's visible only
// when the player owns that vehicle (Subsystem->IsVehicleOwned returns true).
//
// Inherits AInteractableActor — when the player overlaps + presses Interact,
// the slot fires `OnInteract` which (in Blueprint or a subclass) possesses
// the corresponding vehicle Pawn or pops the vehicle-pick widget.

UCLASS(Abstract, Blueprintable)
class TARAGAME_API AVehicleSlotActor : public AInteractableActor
{
	GENERATED_BODY()

public:
	AVehicleSlotActor();

	// Which vehicle this slot represents. Set per-instance in the level.
	// Default Foot keeps slot harmless if designer forgets to set.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tara|Vehicle")
	EVehicleType SlotVehicleType = EVehicleType::Foot;

	// Optional override mesh — if null, falls back to a placeholder cube.
	// Phase 3 visual pass swaps in proper vehicle meshes per type.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tara|Vehicle")
	TObjectPtr<UStaticMeshComponent> VehicleBodyMesh = nullptr;

	// Slot is interactable only when the vehicle is owned. Override from base.
	virtual bool IsInteractAvailable_Implementation() const override;

	// Default OnInteract does nothing — Blueprint override pops the player
	// into the corresponding Pawn (quad bike pawn, horse pawn, etc.) via
	// the level's GameMode / PlayerController.
	virtual bool OnInteract_Implementation(APawn* InstigatorPawn) override;

protected:
	virtual void BeginPlay() override;

private:
	// Refresh mesh visibility + prompt based on current ownership.
	void RefreshSlotState();
};
