#include "Entities/PriceTable.h"

float FPriceTable::SeasonalModifier(ESeason Season) const
{
	return Season == ESeason::Dry ? 0.85f : 1.0f;
}

float FPriceTable::DroughtModifier() const
{
	return FMath::Clamp(1.0f - DroughtSeverity * 0.55f, 0.0f, 1.0f);
}

int32 FPriceTable::CurrentCattlePrice(ESeason Season) const
{
	return FMath::RoundToInt32(BasePerHead * SeasonalModifier(Season) * DroughtModifier());
}

FString FPriceTable::SerializeJson() const
{
	return FString::Printf(
		TEXT("{\"basePerHead\":%d,\"basePerLickDay\":%d,\"basePerMolassesDay\":%d,\"basePerFeedDay\":%d,\"basePerFuelMuster\":%d,\"droughtSeverity\":%.3f}"),
		BasePerHead, BasePerLickDay, BasePerMolassesDay, BasePerFeedDay, BasePerFuelMuster, DroughtSeverity);
}

FPriceTable FPriceTable::FromJson(const FString& Json)
{
	FPriceTable P;

	auto ReadInt = [&](const TCHAR* Key, int32& Out)
	{
		const FString Token = FString::Printf(TEXT("\"%s\":"), Key);
		const int32 Idx = Json.Find(Token);
		if (Idx == INDEX_NONE) return;
		const int32 Start = Idx + Token.Len();
		int32 End = Start;
		while (End < Json.Len() && (FChar::IsDigit(Json[End]) || Json[End] == '-')) End++;
		Out = FCString::Atoi(*Json.Mid(Start, End - Start));
	};
	auto ReadFloat = [&](const TCHAR* Key, float& Out)
	{
		const FString Token = FString::Printf(TEXT("\"%s\":"), Key);
		const int32 Idx = Json.Find(Token);
		if (Idx == INDEX_NONE) return;
		const int32 Start = Idx + Token.Len();
		int32 End = Start;
		while (End < Json.Len() && (FChar::IsDigit(Json[End]) || Json[End] == '.' || Json[End] == '-')) End++;
		Out = FCString::Atof(*Json.Mid(Start, End - Start));
	};

	ReadInt(TEXT("basePerHead"), P.BasePerHead);
	ReadInt(TEXT("basePerLickDay"), P.BasePerLickDay);
	ReadInt(TEXT("basePerMolassesDay"), P.BasePerMolassesDay);
	ReadInt(TEXT("basePerFeedDay"), P.BasePerFeedDay);
	ReadInt(TEXT("basePerFuelMuster"), P.BasePerFuelMuster);
	ReadFloat(TEXT("droughtSeverity"), P.DroughtSeverity);

	return P;
}
