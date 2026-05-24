#include "Actors/BoreActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"

ABoreActor::ABoreActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	WindmillMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WindmillMesh"));
	WindmillMesh->SetupAttachment(Root);
	WindmillMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 350.0f));
	WindmillMesh->SetRelativeScale3D(FVector(0.1f, 0.1f, 7.0f));   // tall thin placeholder

	TankMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankMesh"));
	TankMesh->SetupAttachment(Root);
	TankMesh->SetRelativeLocation(FVector(150.0f, 0.0f, 100.0f));
	TankMesh->SetRelativeScale3D(FVector(1.5f, 1.5f, 2.0f));

	TroughMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TroughMesh"));
	TroughMesh->SetupAttachment(Root);
	TroughMesh->SetRelativeLocation(FVector(280.0f, 0.0f, 30.0f));
	TroughMesh->SetRelativeScale3D(FVector(3.0f, 0.6f, 0.4f));
}
