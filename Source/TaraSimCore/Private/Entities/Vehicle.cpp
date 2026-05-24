#include "Entities/Vehicle.h"

FVehicleStats FVehicle::GetStatsFor(EVehicleType Type)
{
	// Numbers carried byte-for-byte from src/sim/entities/Vehicle.ts PROFILES.
	switch (Type)
	{
	case EVehicleType::Foot:        return { 1.60f, 0.00f,   0, 35,     0 };
	case EVehicleType::Horse:       return { 1.00f, 0.00f,   0, 80,     0 };
	case EVehicleType::TwoWheeler:  return { 0.80f, 0.40f,  15, 40,  4500 };
	case EVehicleType::FourWheeler: return { 0.70f, 0.30f,  20, 55,  8000 };
	case EVehicleType::Buggy:       return { 0.65f, 0.35f,  30, 60, 12000 };
	case EVehicleType::Chopper:     return { 0.35f, 0.00f, 180, 90, 75000 };
	default:                        return { 1.00f, 0.00f,   0, 50,     0 };
	}
}

bool FVehicle::IsWheeled(EVehicleType Type)
{
	return Type == EVehicleType::TwoWheeler
		|| Type == EVehicleType::FourWheeler
		|| Type == EVehicleType::Buggy;
}

TArray<EVehicleType> FVehicle::AllTypes()
{
	return {
		EVehicleType::Foot,
		EVehicleType::Horse,
		EVehicleType::TwoWheeler,
		EVehicleType::FourWheeler,
		EVehicleType::Buggy,
		EVehicleType::Chopper,
	};
}

FVehicle::FVehicle(EVehicleType InType, bool bInOwned)
	: Type(InType)
	, Stats(GetStatsFor(InType))
{
	// Vehicles with zero purchase price are always available (foot, horse).
	bOwned = bInOwned || Stats.PurchasePrice == 0;
}

FString FVehicle::SerializeJson() const
{
	return FString::Printf(
		TEXT("{\"type\":%d,\"condition\":%.2f,\"fuel\":%.2f,\"owned\":%s}"),
		(int32)Type, Condition, Fuel, bOwned ? TEXT("true") : TEXT("false"));
}

FVehicle FVehicle::FromJson(const FString& Json)
{
	int32 TypeInt = 0;
	float Condition = 100.0f;
	float Fuel = 100.0f;
	bool bOwned = false;

	auto ReadInt = [&](const TCHAR* Key, int32& Out)
	{
		const FString Token = FString::Printf(TEXT("\"%s\":"), Key);
		const int32 Idx = Json.Find(Token);
		if (Idx == INDEX_NONE) return;
		int32 P = Idx + Token.Len();
		int32 Start = P;
		while (P < Json.Len() && (FChar::IsDigit(Json[P]) || Json[P] == '-')) P++;
		Out = FCString::Atoi(*Json.Mid(Start, P - Start));
	};
	auto ReadFloat = [&](const TCHAR* Key, float& Out)
	{
		const FString Token = FString::Printf(TEXT("\"%s\":"), Key);
		const int32 Idx = Json.Find(Token);
		if (Idx == INDEX_NONE) return;
		int32 P = Idx + Token.Len();
		int32 Start = P;
		while (P < Json.Len() && (FChar::IsDigit(Json[P]) || Json[P] == '.' || Json[P] == '-')) P++;
		Out = FCString::Atof(*Json.Mid(Start, P - Start));
	};

	ReadInt(TEXT("type"), TypeInt);
	ReadFloat(TEXT("condition"), Condition);
	ReadFloat(TEXT("fuel"), Fuel);

	const int32 OwnedIdx = Json.Find(TEXT("\"owned\":"));
	if (OwnedIdx != INDEX_NONE)
	{
		bOwned = Json.Mid(OwnedIdx + 8, 4).StartsWith(TEXT("true"));
	}

	FVehicle V((EVehicleType)TypeInt, bOwned);
	V.Condition = Condition;
	V.Fuel = Fuel;
	return V;
}
