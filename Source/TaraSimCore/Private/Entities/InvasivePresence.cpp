#include "Entities/InvasivePresence.h"

FString InvasiveSpeciesToString(EInvasiveSpecies Species)
{
	switch (Species)
	{
	case EInvasiveSpecies::Parkinsonia:   return TEXT("parkinsonia");
	case EInvasiveSpecies::RubberVine:    return TEXT("rubber-vine");
	case EInvasiveSpecies::PricklyAcacia: return TEXT("prickly-acacia");
	default: return TEXT("unknown");
	}
}

bool TryParseInvasiveSpecies(const FString& Name, EInvasiveSpecies& OutSpecies)
{
	if (Name == TEXT("parkinsonia")) { OutSpecies = EInvasiveSpecies::Parkinsonia; return true; }
	if (Name == TEXT("rubber-vine")) { OutSpecies = EInvasiveSpecies::RubberVine; return true; }
	if (Name == TEXT("prickly-acacia")) { OutSpecies = EInvasiveSpecies::PricklyAcacia; return true; }
	return false;
}

FString FInvasiveEntry::SerializeJson() const
{
	return FString::Printf(
		TEXT("{\"paddockId\":\"%s\",\"species\":%d,\"coverage\":%.3f}"),
		*PaddockId, (int32)Species, Coverage);
}

FInvasiveEntry FInvasiveEntry::FromJson(const FString& Json)
{
	FInvasiveEntry E;
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
	E.Species = (EInvasiveSpecies)SpeciesInt;
	ReadFloat(TEXT("coverage"), E.Coverage);
	return E;
}
