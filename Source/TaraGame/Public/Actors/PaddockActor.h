#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PaddockActor.generated.h"

class FPaddock;
class UMaterialInstanceDynamic;

/**
 * APaddockActor — the visual + spatial representation of one FPaddock entity.
 * Holds a non-owning pointer to the sim's FPaddock and a reference to the
 * landscape region + dynamic grass material used to render its state.
 *
 * Phase 1 minimum: each tick, read FPaddock->GrassRatio() and push it to the
 * dynamic grass material as a scalar parameter ("GrassFreshness" or similar).
 * The material reacts visually — pale gold full → thin/patchy → bare brown.
 *
 * Phase 2+ extends this to fence + bore visual placement, water-access icons, etc.
 */
UCLASS()
class TARAGAME_API APaddockActor : public AActor
{
	GENERATED_BODY()

public:
	APaddockActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// Which sim paddock does this Actor represent? Set in the Editor or in code
	// when spawning. Lookups go through UTaraSimSubsystem.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tara")
	FString PaddockId;

	// Static mesh ground plane (placeholder for the Landscape region in Phase 1).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tara")
	TObjectPtr<class UStaticMeshComponent> GroundMesh;

	// Optional explicit material — if unset, BeginPlay grabs whatever GroundMesh
	// has and creates a Dynamic Material Instance from it.
	UPROPERTY(EditAnywhere, Category = "Tara")
	TObjectPtr<class UMaterialInterface> GrassMaterial;

	// Dynamic material instance — what we actually push grass-ratio updates to.
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GrassMID;

private:
	const FPaddock* GetSimPaddock() const;

	float CachedGrassRatio = 1.0f;
};
