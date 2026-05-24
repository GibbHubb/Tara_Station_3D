#include "Entities/Player.h"

FPlayer::FPlayer(const FPlayerConfig& Config)
	: Role(Config.Role)
	, Cash(Config.Cash)
	, Skills(Config.Skills)
{
}

FString FPlayer::SerializeJson() const
{
	return FString::Printf(
		TEXT("{\"role\":%d,\"cash\":%d,\"daysOnStation\":%d,\"skills\":{\"mustering\":%.2f,\"mechanics\":%.2f,\"animals\":%.2f,\"birdwatching\":%.2f,\"shooting\":%.2f}}"),
		(int32)Role, Cash, DaysOnStation,
		Skills.Mustering, Skills.Mechanics, Skills.Animals, Skills.Birdwatching, Skills.Shooting);
}

FPlayer FPlayer::FromJson(const FString& Json)
{
	FPlayer P;

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

	int32 RoleInt = 0;
	ReadInt(TEXT("role"), RoleInt);
	P.Role = (EPlayerRole)RoleInt;
	ReadInt(TEXT("cash"), P.Cash);
	ReadInt(TEXT("daysOnStation"), P.DaysOnStation);

	ReadFloat(TEXT("mustering"), P.Skills.Mustering);
	ReadFloat(TEXT("mechanics"), P.Skills.Mechanics);
	ReadFloat(TEXT("animals"), P.Skills.Animals);
	ReadFloat(TEXT("birdwatching"), P.Skills.Birdwatching);
	ReadFloat(TEXT("shooting"), P.Skills.Shooting);

	return P;
}
