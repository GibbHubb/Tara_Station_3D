#include "Actors/PaddockActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Bridge/TaraSimSubsystem.h"
#include "Entities/Paddock.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

APaddockActor::APaddockActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.5f;   // we don't need 60Hz; half a second is fine.

	GroundMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundMesh"));
	RootComponent = GroundMesh;
	GroundMesh->SetMobility(EComponentMobility::Static);
}

void APaddockActor::BeginPlay()
{
	Super::BeginPlay();

	// Create a dynamic material instance so we can push grass-ratio per-tick.
	if (UMaterialInterface* Source = GrassMaterial ? GrassMaterial.Get() : GroundMesh->GetMaterial(0))
	{
		GrassMID = UMaterialInstanceDynamic::Create(Source, this);
		GroundMesh->SetMaterial(0, GrassMID);
	}
}

void APaddockActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const FPaddock* P = GetSimPaddock();
	if (!P) return;

	const float Ratio = P->GrassRatio();
	if (FMath::IsNearlyEqual(Ratio, CachedGrassRatio, 0.01f)) return;
	CachedGrassRatio = Ratio;

	if (GrassMID)
	{
		// "GrassFreshness" — scalar parameter name in the Master_Grass material.
		// 1.0 = full pale gold; 0.0 = bare patches. The material does the rest.
		GrassMID->SetScalarParameterValue(TEXT("GrassFreshness"), Ratio);
	}
}

const FPaddock* APaddockActor::GetSimPaddock() const
{
	if (UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UTaraSimSubsystem* Sim = GI->GetSubsystem<UTaraSimSubsystem>())
		{
			if (const FStation* S = Sim->GetStation())
			{
				return S->PaddockById(PaddockId);
			}
		}
	}
	return nullptr;
}
