#pragma once

#include "CoreMinimal.h"

// Mirrors src/sim/entities/Species.ts. Bird + wildlife species registry —
// 13 birds + 3 mammals on M6 ship; expandable.
//
// Per [world.md](docs/product/world.md), this list aligns with the M6 bird
// researcher logbook. Rarities drive the bird-sighting roll weighting in
// WildlifeSystem (common 6× / uncommon 2× / rare 1×).

enum class ESpeciesGroup : uint8
{
	Bird   = 0,
	Mammal = 1,
};

enum class ESpeciesHabitat : uint8
{
	Plains = 0,
	Creek  = 1,
	Trees  = 2,
	Any    = 3,
};

enum class ESpeciesRarity : uint8
{
	Common   = 0,
	Uncommon = 1,
	Rare     = 2,
};

struct FSpeciesDef
{
	FString Id;
	FString Name;
	ESpeciesGroup Group = ESpeciesGroup::Bird;
	ESpeciesHabitat Habitat = ESpeciesHabitat::Any;
	ESpeciesRarity Rarity = ESpeciesRarity::Common;
	int32 ResearcherPayout = 0;
};

class TARASIMCORE_API FSpeciesRegistry
{
public:
	// Returns the global bird list (13 species), mammals (3), and all together.
	static const TArray<FSpeciesDef>& Birds();
	static const TArray<FSpeciesDef>& Mammals();
	static const TArray<FSpeciesDef>& All();

	// Lookup by id; returns nullptr if not found.
	static const FSpeciesDef* SpeciesById(const FString& Id);
};
