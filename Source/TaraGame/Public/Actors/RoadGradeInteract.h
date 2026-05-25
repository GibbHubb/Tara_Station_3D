#pragma once

#include "CoreMinimal.h"
#include "Actors/InteractableActor.h"
#include "RoadGradeInteract.generated.h"

// Bonus subclass beyond TS-3D-RINGER-WORK plan §3 — road grading.
// Same pattern as fence repair. Place at a road end-point. Prompt visible
// when road's gradeQuality < GradeThreshold (default 50 — below 60
// penalises wheeled vehicles per M4a; below 50 is "needs grading").
// OnInteract calls `Subsystem->GradeRoad(RoadId)`.

UCLASS(Blueprintable)
class TARAGAME_API ARoadGradeInteract : public AInteractableActor
{
	GENERATED_BODY()

public:
	ARoadGradeInteract();

	// Must match a real road id (e.g. "road-Holding-Mid-II").
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tara|Road")
	FString RoadId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tara|Road")
	float GradeThreshold = 50.0f;

	virtual bool IsInteractAvailable_Implementation() const override;
	virtual bool OnInteract_Implementation(APawn* InstigatorPawn) override;
};
