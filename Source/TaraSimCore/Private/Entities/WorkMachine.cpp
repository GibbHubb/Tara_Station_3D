#include "Entities/WorkMachine.h"

FWorkMachineStats FWorkMachine::GetStatsFor(EWorkMachineType Type)
{
	// Byte-for-byte from src/sim/entities/WorkMachine.ts PROFILES.
	switch (Type)
	{
	case EWorkMachineType::Tractor:        return { 25000, 20, TEXT("General-purpose workhorse"), TEXT("general work") };
	case EWorkMachineType::FrontEndLoader: return { 38000, 30, TEXT("Lifts and moves"),           TEXT("lifting + moving") };
	case EWorkMachineType::Forklift:       return { 18000, 10, TEXT("Yard work"),                 TEXT("yard work") };
	case EWorkMachineType::Digger:         return { 75000, 50, TEXT("Bore work, dam clean-out"),  TEXT("earthworks") };
	case EWorkMachineType::Grader:         return { 95000, 40, TEXT("Grades roads — speeds up wheeled musters"), TEXT("grade roads") };
	case EWorkMachineType::FlatBedTrailer: return {  8000,  5, TEXT("Carting supplies"),          TEXT("cart supplies") };
	case EWorkMachineType::MolassesTruck:  return { 32000, 25, TEXT("Supplement delivery"),       TEXT("fast molasses delivery") };
	default: return { 0, 0, TEXT(""), TEXT("") };
	}
}

TArray<EWorkMachineType> FWorkMachine::AllTypes()
{
	return {
		EWorkMachineType::Tractor,
		EWorkMachineType::FrontEndLoader,
		EWorkMachineType::Forklift,
		EWorkMachineType::Digger,
		EWorkMachineType::Grader,
		EWorkMachineType::FlatBedTrailer,
		EWorkMachineType::MolassesTruck,
	};
}

FWorkMachine::FWorkMachine(EWorkMachineType InType, bool bInOwned)
	: Type(InType)
	, Stats(GetStatsFor(InType))
	, bOwned(bInOwned)
{
}

FString FWorkMachine::SerializeJson() const
{
	return FString::Printf(
		TEXT("{\"type\":%d,\"condition\":%.2f,\"fuel\":%.2f,\"owned\":%s}"),
		(int32)Type, Condition, Fuel, bOwned ? TEXT("true") : TEXT("false"));
}

FWorkMachine FWorkMachine::FromJson(const FString& Json)
{
	int32 TypeInt = 0;
	float Condition = 100.0f;
	float Fuel = 100.0f;
	bool bOwned = false;

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

	ReadInt(TEXT("type"), TypeInt);
	ReadFloat(TEXT("condition"), Condition);
	ReadFloat(TEXT("fuel"), Fuel);

	const int32 OIdx = Json.Find(TEXT("\"owned\":"));
	if (OIdx != INDEX_NONE)
	{
		bOwned = Json.Mid(OIdx + 8, 4).StartsWith(TEXT("true"));
	}

	FWorkMachine M((EWorkMachineType)TypeInt, bOwned);
	M.Condition = Condition;
	M.Fuel = Fuel;
	return M;
}
