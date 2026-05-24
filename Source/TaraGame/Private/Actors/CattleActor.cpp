#include "Actors/CattleActor.h"
#include "Components/StaticMeshComponent.h"

ACattleActor::ACattleActor()
{
	PrimaryActorTick.bCanEverTick = false;

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	RootComponent = BodyMesh;
	BodyMesh->SetRelativeScale3D(FVector(1.5f, 0.6f, 1.0f));   // bovine-ish placeholder proportions
}
