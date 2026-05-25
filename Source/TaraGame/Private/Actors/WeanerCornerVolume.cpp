#include "Actors/WeanerCornerVolume.h"
#include "Bridge/TaraSimSubsystem.h"
#include "Station.h"
#include "EventBus.h"
#include "Entities/CattleCohort.h"
#include "Entities/Herd.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

AWeanerCornerVolume::AWeanerCornerVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	Volume = CreateDefaultSubobject<UBoxComponent>(TEXT("Volume"));
	RootComponent = Volume;
	Volume->SetCollisionProfileName(TEXT("NoCollision"));
	Volume->SetBoxExtent(FVector(1500.0f, 1500.0f, 500.0f));
}

void AWeanerCornerVolume::BeginPlay()
{
	Super::BeginPlay();

	UTaraSimSubsystem* Sub = GetSubsystem();
	if (Sub && Sub->GetStationMutable())
	{
		FEventBus& Bus = Sub->GetStationMutable()->GetBus();
		SubIdDayEnded = Bus.DayEnded.Subscribe(
			[this](const FDayEndedPayload&) { HandleDayEnded(); });
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[WeanerCornerVolume] no subsystem at BeginPlay — schooling won't run"));
	}
}

void AWeanerCornerVolume::EndPlay(const EEndPlayReason::Type Reason)
{
	if (SubIdDayEnded != -1)
	{
		UTaraSimSubsystem* Sub = GetSubsystem();
		if (Sub && Sub->GetStationMutable())
		{
			Sub->GetStationMutable()->GetBus().DayEnded.Unsubscribe(SubIdDayEnded);
		}
		SubIdDayEnded = -1;
	}
	Super::EndPlay(Reason);
}

void AWeanerCornerVolume::HandleDayEnded()
{
	UTaraSimSubsystem* Sub = GetSubsystem();
	if (!Sub || !Sub->GetStationMutable() || PaddockId.IsEmpty()) return;

	FStation* Station = Sub->GetStationMutable();
	if (Station->GetHerd().CurrentPaddockId != PaddockId) return;

	// Find any Weaner-state cohort in the herd; rise its trainedness.
	for (FCattleCohort& C : Station->GetHerd().Cohorts)
	{
		if (C.State == ECohortState::Weaner && C.Count > 0)
		{
			C.MusterTrainedness = FMath::Min(100.0f,
				C.MusterTrainedness + TrainednessRiseDailyAmount);
		}
	}
}

UTaraSimSubsystem* AWeanerCornerVolume::GetSubsystem() const
{
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(GetWorld()))
	{
		return GI->GetSubsystem<UTaraSimSubsystem>();
	}
	return nullptr;
}
