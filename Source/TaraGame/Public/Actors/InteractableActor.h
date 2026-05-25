#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableActor.generated.h"

class UBoxComponent;
class UTextRenderComponent;

// Shared base for world-space player-interactable actors per TS-3D-RINGER-WORK
// plan §5. One overlap trigger volume + a prompt label + an Interact key
// binding. Subclasses override OnInteract to fire their specific bridge call
// (e.g. CheckBore, RepairFence, ShootPest).
//
// The base is engine-aware (lives in TaraGame). It pulls the player Pawn
// from the overlap event + binds the Interact key via the player's
// EnhancedInput context — your level/GameMode is responsible for granting
// the InteractAction binding to the player (see PHASE2_RINGER_WORK_EDITOR_TODO
// when it's written).

UCLASS(Abstract, Blueprintable)
class TARAGAME_API AInteractableActor : public AActor
{
	GENERATED_BODY()

public:
	AInteractableActor();

	// Subclasses override this to fire the specific bridge action. Returns
	// true if the action succeeded (subclass-defined). Default = false.
	// Implementable in Blueprint for designer-side wiring.
	UFUNCTION(BlueprintNativeEvent, Category = "Tara|Interact")
	bool OnInteract(APawn* InstigatorPawn);
	virtual bool OnInteract_Implementation(APawn* InstigatorPawn);

	// Subclasses override to control whether the prompt is currently visible.
	// E.g. a bore-check prompt only shows when bore condition < 50. Default
	// returns true (always visible when player overlaps).
	UFUNCTION(BlueprintNativeEvent, Category = "Tara|Interact")
	bool IsInteractAvailable() const;
	virtual bool IsInteractAvailable_Implementation() const;

	// The verb shown in the prompt ("Check bore", "Repair fence"). Designer-
	// editable per instance.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tara|Interact")
	FText PromptVerb = NSLOCTEXT("Tara", "InteractDefaultVerb", "Interact");

protected:
	virtual void BeginPlay() override;

	// Box trigger — player overlaps to see the prompt.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tara|Interact")
	TObjectPtr<UBoxComponent> TriggerBox = nullptr;

	// World-space text label (placeholder; Phase 3 visual pass swaps for a
	// proper UMG world widget). Cheaper for Phase 2 than a screen-space widget.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tara|Interact")
	TObjectPtr<UTextRenderComponent> PromptLabel = nullptr;

	UFUNCTION()
	void HandleOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	// The pawn currently inside the trigger volume (if any). The level's
	// EnhancedInput InteractAction handler calls AInteractableActor::TryInteract
	// on this when the key is pressed.
	UPROPERTY(Transient)
	TWeakObjectPtr<APawn> CurrentOverlappingPawn;

public:
	// Called by the player's input handler when the Interact key fires.
	// Resolves to the nearest available interactable + fires OnInteract.
	// Static helper for convenience in the player Pawn's Blueprint.
	UFUNCTION(BlueprintCallable, Category = "Tara|Interact")
	static bool TryInteractWithNearest(APawn* InstigatorPawn);

	// Refreshes the prompt label visibility — call after sim state changes
	// that might flip IsInteractAvailable (e.g. bore got fixed).
	UFUNCTION(BlueprintCallable, Category = "Tara|Interact")
	void RefreshPrompt();
};
