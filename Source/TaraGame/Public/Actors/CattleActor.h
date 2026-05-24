#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CattleActor.generated.h"

/**
 * ACattleActor — one head of cattle visible in the world. Phase 1 = a static
 * placeholder mesh that stands in the paddock (no flocking, no grazing
 * animation yet). Phase 2 adds flocking AI per the herd-AI design.
 *
 * The sim's count of cattle is the source of truth; this Actor is one of
 * ~10-30 visible "representatives" drawn from a cohort. At higher head counts
 * the visible count saturates while the sim continues to track the real number.
 */
UCLASS()
class TARAGAME_API ACattleActor : public AActor
{
	GENERATED_BODY()

public:
	ACattleActor();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tara")
	FString PaddockId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tara")
	TObjectPtr<class UStaticMeshComponent> BodyMesh;
};
