#pragma once

#include "CoreMinimal.h"
#include "Actors/InteractableActor.h"
#include "BoreCheckInteract.generated.h"

// TS-3D-RINGER-WORK plan §3 — first of the 7 ringer-work interactables.
// Attach as a child of `BoreActor` (or place adjacent). Prompt visible when
// bore condition < ConditionThreshold (default 50). OnInteract calls
// `Subsystem->CheckBore(BoreId)` which fires the 80%-success repair roll.
//
// BP_BoreInteract subclass binds the `BoreId` per-instance in the level —
// must match a sim-side bore id from `Station::SeedDefaults`.

UCLASS(Blueprintable)
class TARAGAME_API ABoreCheckInteract : public AInteractableActor
{
	GENERATED_BODY()

public:
	ABoreCheckInteract();

	// Must match a real bore id (e.g. "Holding-Paddock-Nest").
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tara|Bore")
	FString BoreId;

	// Show prompt only when bore condition is below this. Default 50 — half-
	// degraded; player should preemptively fix before failure.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tara|Bore")
	float ConditionThreshold = 50.0f;

	virtual bool IsInteractAvailable_Implementation() const override;
	virtual bool OnInteract_Implementation(APawn* InstigatorPawn) override;
};
