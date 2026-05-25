#pragma once

#include "CoreMinimal.h"
#include "Actors/InteractableActor.h"
#include "FenceRepairInteract.generated.h"

// TS-3D-RINGER-WORK plan §3 — fence-repair interactable. Place adjacent to a
// fence section. Prompt visible when fence integrity < IntegrityThreshold
// (default 50). OnInteract calls `Subsystem->RepairFence(FenceId)`.

UCLASS(Blueprintable)
class TARAGAME_API AFenceRepairInteract : public AInteractableActor
{
	GENERATED_BODY()

public:
	AFenceRepairInteract();

	// Must match a real fence id (e.g. "fence-Holding-Mid-II").
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tara|Fence")
	FString FenceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tara|Fence")
	float IntegrityThreshold = 50.0f;

	virtual bool IsInteractAvailable_Implementation() const override;
	virtual bool OnInteract_Implementation(APawn* InstigatorPawn) override;
};
