#include "Entities/PestPresence.h"

FPestImpact GetPestImpact(EPestSpecies Species)
{
	switch (Species)
	{
	case EPestSpecies::Dingo:    return { 0.012f, TEXT("preys on calves") };
	case EPestSpecies::FeralCat: return { 0.0f,   TEXT("kills birdlife") };
	case EPestSpecies::WildPig:  return { 0.004f, TEXT("damages paddocks, rooting up grass") };
	default: return { 0.0f, TEXT("") };
	}
}

FString PestSpeciesToString(EPestSpecies Species)
{
	switch (Species)
	{
	case EPestSpecies::Dingo:    return TEXT("dingo");
	case EPestSpecies::FeralCat: return TEXT("feral-cat");
	case EPestSpecies::WildPig:  return TEXT("wild-pig");
	default: return TEXT("unknown");
	}
}

bool TryParsePestSpecies(const FString& Name, EPestSpecies& OutSpecies)
{
	if (Name == TEXT("dingo")) { OutSpecies = EPestSpecies::Dingo; return true; }
	if (Name == TEXT("feral-cat")) { OutSpecies = EPestSpecies::FeralCat; return true; }
	if (Name == TEXT("wild-pig")) { OutSpecies = EPestSpecies::WildPig; return true; }
	return false;
}

FString FPestEntry::SerializeJson() const
{
	return FString::Printf(
		TEXT("{\"paddockId\":\"%s\",\"species\":%d,\"population\":%.3f}"),
		*PaddockId, (int32)Species, Population);
}

FPestEntry FPestEntry::FromJson(const FString& Json)
{
	FPestEntry E;
	const FString Tok = TEXT("\"paddockId\":\"");
	const int32 Idx = Json.Find(Tok);
	if (Idx != INDEX_NONE)
	{
		const int32 Start = Idx + Tok.Len();
		const int32 End = Json.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, Start);
		if (End != INDEX_NONE) E.PaddockId = Json.Mid(Start, End - Start);
	}
	auto ReadInt = [&](const TCHAR* Key, int32& Out)
	{
		const FString T = FString::Printf(TEXT("\"%s\":"), Key);
		const int32 II = Json.Find(T);
		if (II == INDEX_NONE) return;
		int32 P = II + T.Len();
		int32 S = P;
		while (P < Json.Len() && (FChar::IsDigit(Json[P]) || Json[P] == '-')) P++;
		Out = FCString::Atoi(*Json.Mid(S, P - S));
	};
	auto ReadFloat = [&](const TCHAR* Key, float& Out)
	{
		const FString T = FString::Printf(TEXT("\"%s\":"), Key);
		const int32 II = Json.Find(T);
		if (II == INDEX_NONE) return;
		int32 P = II + T.Len();
		int32 S = P;
		while (P < Json.Len() && (FChar::IsDigit(Json[P]) || Json[P] == '.' || Json[P] == '-')) P++;
		Out = FCString::Atof(*Json.Mid(S, P - S));
	};
	int32 SpeciesInt = 0;
	ReadInt(TEXT("species"), SpeciesInt);
	E.Species = (EPestSpecies)SpeciesInt;
	ReadFloat(TEXT("population"), E.Population);
	return E;
}
