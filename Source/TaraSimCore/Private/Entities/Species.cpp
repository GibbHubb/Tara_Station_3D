#include "Entities/Species.h"

namespace
{
	TArray<FSpeciesDef> MakeBirds()
	{
		// Carries byte-for-byte from src/sim/entities/Species.ts BIRD_SPECIES.
		return {
			{ TEXT("emu"),               TEXT("Emu"),                       ESpeciesGroup::Bird, ESpeciesHabitat::Plains, ESpeciesRarity::Common,    50 },
			{ TEXT("brolga"),            TEXT("Brolga"),                    ESpeciesGroup::Bird, ESpeciesHabitat::Plains, ESpeciesRarity::Uncommon, 120 },
			{ TEXT("kookaburra"),        TEXT("Kookaburra"),                ESpeciesGroup::Bird, ESpeciesHabitat::Trees,  ESpeciesRarity::Common,    40 },
			{ TEXT("galah"),             TEXT("Galah"),                     ESpeciesGroup::Bird, ESpeciesHabitat::Trees,  ESpeciesRarity::Common,    30 },
			{ TEXT("cockatoo"),          TEXT("Sulphur-crested Cockatoo"),  ESpeciesGroup::Bird, ESpeciesHabitat::Trees,  ESpeciesRarity::Common,    40 },
			{ TEXT("rainbow-lorikeet"),  TEXT("Rainbow Lorikeet"),          ESpeciesGroup::Bird, ESpeciesHabitat::Trees,  ESpeciesRarity::Common,    35 },
			{ TEXT("willie-wagtail"),    TEXT("Willie Wagtail"),            ESpeciesGroup::Bird, ESpeciesHabitat::Any,    ESpeciesRarity::Common,    30 },
			{ TEXT("kingfisher"),        TEXT("Azure Kingfisher"),          ESpeciesGroup::Bird, ESpeciesHabitat::Creek,  ESpeciesRarity::Uncommon, 110 },
			{ TEXT("white-faced-heron"), TEXT("White-faced Heron"),         ESpeciesGroup::Bird, ESpeciesHabitat::Creek,  ESpeciesRarity::Common,    45 },
			{ TEXT("jabiru"),            TEXT("Jabiru"),                    ESpeciesGroup::Bird, ESpeciesHabitat::Creek,  ESpeciesRarity::Uncommon, 150 },
			{ TEXT("wedge-tailed-eagle"),TEXT("Wedge-tailed Eagle"),        ESpeciesGroup::Bird, ESpeciesHabitat::Any,    ESpeciesRarity::Uncommon, 200 },
			{ TEXT("night-parrot"),      TEXT("Night Parrot"),              ESpeciesGroup::Bird, ESpeciesHabitat::Plains, ESpeciesRarity::Rare,    1500 },
			{ TEXT("gouldian-finch"),    TEXT("Gouldian Finch"),            ESpeciesGroup::Bird, ESpeciesHabitat::Trees,  ESpeciesRarity::Rare,     800 },
		};
	}

	TArray<FSpeciesDef> MakeMammals()
	{
		return {
			{ TEXT("red-roo"),  TEXT("Red Kangaroo"), ESpeciesGroup::Mammal, ESpeciesHabitat::Plains, ESpeciesRarity::Common,    0 },
			{ TEXT("wombat"),   TEXT("Wombat"),       ESpeciesGroup::Mammal, ESpeciesHabitat::Trees,  ESpeciesRarity::Uncommon,  0 },
			{ TEXT("echidna"),  TEXT("Echidna"),      ESpeciesGroup::Mammal, ESpeciesHabitat::Any,    ESpeciesRarity::Common,    0 },
		};
	}

	TArray<FSpeciesDef> MakeAll(const TArray<FSpeciesDef>& B, const TArray<FSpeciesDef>& M)
	{
		TArray<FSpeciesDef> Out = B;
		Out.Append(M);
		return Out;
	}
}

const TArray<FSpeciesDef>& FSpeciesRegistry::Birds()
{
	static const TArray<FSpeciesDef> B = MakeBirds();
	return B;
}

const TArray<FSpeciesDef>& FSpeciesRegistry::Mammals()
{
	static const TArray<FSpeciesDef> M = MakeMammals();
	return M;
}

const TArray<FSpeciesDef>& FSpeciesRegistry::All()
{
	static const TArray<FSpeciesDef> A = MakeAll(Birds(), Mammals());
	return A;
}

const FSpeciesDef* FSpeciesRegistry::SpeciesById(const FString& Id)
{
	for (const FSpeciesDef& S : All())
	{
		if (S.Id == Id) return &S;
	}
	return nullptr;
}
