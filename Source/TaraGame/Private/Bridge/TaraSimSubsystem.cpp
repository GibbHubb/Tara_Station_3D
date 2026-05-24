#include "Bridge/TaraSimSubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

void UTaraSimSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (!TryLoadFromDisk())
	{
		NewGame();
	}

	WireSimEventsToDelegates();

	UE_LOG(LogTemp, Display, TEXT("[TaraSim] Initialized. Year %d Day %d %s — herd %d head"),
		Station->GetClock().Year, Station->GetClock().DayOfYear,
		*Station->GetClock().FormatTime(), Station->GetHerd().GetSize());
}

void UTaraSimSubsystem::Deinitialize()
{
	UnwireSimEvents();
	WriteSaveToDisk();
	Station.Reset();
	Super::Deinitialize();
}

void UTaraSimSubsystem::TickDay()
{
	if (!Station) return;
	Station->TickDay();
	// Autosave on every day-tick — mirrors 2D Save-on-DayEnded.
	// The DayEnded event handler also writes; this is belt-and-braces during Phase 0.
}

void UTaraSimSubsystem::SaveNow()
{
	WriteSaveToDisk();
}

void UTaraSimSubsystem::NewGame()
{
	UnwireSimEvents();
	Station = MakeUnique<FStation>();
	WireSimEventsToDelegates();
}

int32 UTaraSimSubsystem::GetYear() const
{
	return Station ? Station->GetClock().Year : 0;
}

int32 UTaraSimSubsystem::GetDayOfYear() const
{
	return Station ? Station->GetClock().DayOfYear : 0;
}

FString UTaraSimSubsystem::GetFormattedTime() const
{
	return Station ? Station->GetClock().FormatTime() : TEXT("--:--");
}

int32 UTaraSimSubsystem::GetHerdSize() const
{
	return Station ? Station->GetHerd().GetSize() : 0;
}

float UTaraSimSubsystem::GetHerdConditionMean() const
{
	return Station ? Station->GetHerd().GetConditionMean() : 0.0f;
}

float UTaraSimSubsystem::GetPaddockGrassKgPerHa(const FString& PaddockId) const
{
	if (!Station) return 0.0f;
	const FPaddock* P = Station->PaddockById(PaddockId);
	return P ? P->GrassKgPerHa : 0.0f;
}

void UTaraSimSubsystem::WireSimEventsToDelegates()
{
	if (!Station) return;
	FEventBus& Bus = Station->GetBus();

	SubIdDayEnded = Bus.DayEnded.Subscribe([this](const FDayEndedPayload& P)
	{
		OnDayEnded.Broadcast(P);
		// Autosave on every DayEnded.
		WriteSaveToDisk();
	});

	SubIdCattleDied = Bus.CattleDied.Subscribe([this](const FCattleDiedPayload& P)
	{
		OnCattleDied.Broadcast(P);
		UE_LOG(LogTemp, Warning, TEXT("[TaraSim] CattleDied: %d head — %s"), P.Count, *P.Cause);
	});

	SubIdHerdCondition = Bus.HerdConditionChanged.Subscribe([this](const FHerdConditionChangedPayload& P)
	{
		OnHerdConditionChanged.Broadcast(P);
	});
}

void UTaraSimSubsystem::UnwireSimEvents()
{
	if (!Station) return;
	FEventBus& Bus = Station->GetBus();
	if (SubIdDayEnded != -1)      { Bus.DayEnded.Unsubscribe(SubIdDayEnded); SubIdDayEnded = -1; }
	if (SubIdCattleDied != -1)    { Bus.CattleDied.Unsubscribe(SubIdCattleDied); SubIdCattleDied = -1; }
	if (SubIdHerdCondition != -1) { Bus.HerdConditionChanged.Unsubscribe(SubIdHerdCondition); SubIdHerdCondition = -1; }
}

FString UTaraSimSubsystem::GetSaveFilePath() const
{
	return FPaths::ProjectSavedDir() / TEXT("tara-save-3d-v1-phase0.json");
}

bool UTaraSimSubsystem::TryLoadFromDisk()
{
	const FString Path = GetSaveFilePath();
	FString Json;
	if (!FFileHelper::LoadFileToString(Json, *Path))
	{
		return false;
	}

	TUniquePtr<FStation> Loaded = FStation::FromJson(Json);
	if (!Loaded)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TaraSim] Save schema mismatch or parse failure — discarding."));
		return false;
	}

	Station = MoveTemp(Loaded);
	return true;
}

void UTaraSimSubsystem::WriteSaveToDisk() const
{
	if (!Station) return;
	const FString Path = GetSaveFilePath();
	const FString Json = Station->SerializeJson();
	if (!FFileHelper::SaveStringToFile(Json, *Path))
	{
		UE_LOG(LogTemp, Warning, TEXT("[TaraSim] Save failed: %s"), *Path);
	}
}
