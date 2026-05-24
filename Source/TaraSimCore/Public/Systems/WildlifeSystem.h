#pragma once

#include "CoreMinimal.h"
#include "../Entities/Species.h"
#include "../Entities/PestPresence.h"
#include "../Entities/InvasivePresence.h"

class FStation;
class FEventBus;

// Mirrors src/sim/systems/WildlifeSystem.ts. M6 wildlife layer:
//   • Pests (dingo / feral cat / wild pig) populate per paddock, grow 2%/d,
//     can take calves when the herd shares the paddock.
//   • Invasives (parkinsonia / rubber vine / prickly acacia) spread 0.05%/d
//     coverage; jump to adjacent paddocks at high coverage occasionally.
//   • Bird sightings — weighted roll by habitat + rarity; researcher pays a
//     one-time bounty on first log per species.
//
// In 3D (Phase 5+): pest/invasive presence is exposed as a per-paddock state
// the visualisation layer reads (paddock material gets a "infested" overlay
// at >40% coverage; pest icons spawn at paddock edges). Bird sightings flow
// through 3D encounter triggers as the player traverses.

class TARASIMCORE_API FWildlifeSystem
{
public:
	FWildlifeSystem(FStation& InStation, FEventBus& InBus);

	void InitDefaults();
	void TickDay();

	// Bird sighting roll — picks a random species weighted by rarity given the
	// habitat in front of the player. Returns nullptr if no candidates.
	const FSpeciesDef* RollBirdSighting(ESpeciesHabitat Habitat) const;

	// Log a bird — emits BirdLogged + pays researcher bounty on first log.
	struct FLogResult { int32 Payout = 0; bool bFirst = false; };
	FLogResult LogBird(const FString& SpeciesId);

	// Shoot a pest. Skill-gated; returns true if a head was taken. Bumps shooting
	// skill on success. Mirrors 2D.
	bool ShootPest(const FString& PaddockId, EPestSpecies Species);

	// Treat an invasive patch — knocks coverage down by 25.
	bool TreatInvasive(const FString& PaddockId, EInvasiveSpecies Species);

	const TArray<FPestEntry>& GetPests() const { return Pests; }
	const TArray<FInvasiveEntry>& GetInvasives() const { return Invasives; }
	const TSet<FString>& GetLoggedSpeciesIds() const { return LoggedSpeciesIds; }
	int32 GetLoggedSpeciesCount() const { return LoggedSpeciesIds.Num(); }

	FString SerializeJson() const;
	void LoadFromJson(const FString& Json);

	// Constants — byte-for-byte from 2D.
	static constexpr float PestGrowthPerDay = 0.02f;
	static constexpr float InvasiveSpreadPerDay = 0.05f;

private:
	FStation& Station;
	FEventBus& Bus;

	TArray<FPestEntry> Pests;
	TArray<FInvasiveEntry> Invasives;
	TSet<FString> LoggedSpeciesIds;
};
