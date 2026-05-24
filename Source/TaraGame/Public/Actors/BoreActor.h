#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BoreActor.generated.h"

/**
 * ABoreActor — the visual representation of a bore. Phase 1 minimum: windmill
 * mesh + concrete trough mesh, sitting at the actor's transform.
 *
 * Phase 2 wires this to FBore in TaraSimCore (M2 water): condition + status.
 * For Phase 1 the bore exists only visually; the sim doesn't know about it yet.
 */
UCLASS()
class TARAGAME_API ABoreActor : public AActor
{
	GENERATED_BODY()

public:
	ABoreActor();

	// Bore identifier matching FBore.Id in TaraSimCore (set when M2 sim lands).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tara")
	FString BoreId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tara")
	TObjectPtr<class USceneComponent> Root;

	// Windmill silhouette — placeholder Cylinder in Phase 1.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tara")
	TObjectPtr<class UStaticMeshComponent> WindmillMesh;

	// Concrete trough — placeholder Cube.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tara")
	TObjectPtr<class UStaticMeshComponent> TroughMesh;

	// Tank — placeholder Cylinder.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tara")
	TObjectPtr<class UStaticMeshComponent> TankMesh;
};
