#include "Actors/VehicleSlotActor.h"
#include "Bridge/TaraSimSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

AVehicleSlotActor::AVehicleSlotActor()
{
	// Default prompt verb. Designer can override per-instance.
	PromptVerb = NSLOCTEXT("Tara", "TakeVehiclePromptVerb", "Take vehicle");

	VehicleBodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VehicleBodyMesh"));
	VehicleBodyMesh->SetupAttachment(RootComponent);
	VehicleBodyMesh->SetCollisionProfileName(TEXT("NoCollision"));
	// No mesh assigned by default — designer sets per slot.
}

void AVehicleSlotActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshSlotState();
}

bool AVehicleSlotActor::IsInteractAvailable_Implementation() const
{
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(GetWorld()))
	{
		if (UTaraSimSubsystem* Sub = GI->GetSubsystem<UTaraSimSubsystem>())
		{
			return Sub->IsVehicleOwned((int32)SlotVehicleType);
		}
	}
	return false;
}

bool AVehicleSlotActor::OnInteract_Implementation(APawn* /*InstigatorPawn*/)
{
	// Default: log + return true. Blueprint override does the possession
	// swap to the actual vehicle Pawn (BP_QuadBike, BP_Bike, etc.).
	UE_LOG(LogTemp, Display, TEXT("[VehicleSlot] Player chose vehicle type %d (override OnInteract in BP to handle)"),
		(int32)SlotVehicleType);
	return true;
}

void AVehicleSlotActor::RefreshSlotState()
{
	const bool bOwned = IsInteractAvailable();
	if (VehicleBodyMesh)
	{
		VehicleBodyMesh->SetVisibility(bOwned);
	}
	RefreshPrompt();
}
