#pragma once

#include "CoreMinimal.h"

// Mirrors src/sim/entities/Player.ts. Pure data + role state machine.
// Ports unchanged in logic per sim-port-audit.md (the cleanest port in the
// codebase — every field carries over).

enum class EPlayerRole : uint8
{
	Ringer,
	Manager,
	Owner,
};

struct FPlayerSkills
{
	float Mustering = 30.0f;     // 0..100; rises with successful musters (M4)
	float Mechanics = 20.0f;     // 0..100; affects bush-mechanic fixes (M8)
	float Animals = 30.0f;       // 0..100; affects condition recovery + breeding (M5)
	float Birdwatching = 10.0f;  // 0..100; researcher payouts (M6)
	float Shooting = 20.0f;      // 0..100; pest control accuracy (M6)
};

struct FPlayerConfig
{
	EPlayerRole Role = EPlayerRole::Ringer;
	int32 Cash = 250;
	FPlayerSkills Skills;
};

class TARASIMCORE_API FPlayer
{
public:
	FPlayer() = default;
	explicit FPlayer(const FPlayerConfig& Config);

	FString SerializeJson() const;
	static FPlayer FromJson(const FString& Json);

public:
	EPlayerRole Role = EPlayerRole::Ringer;
	int32 Cash = 250;
	FPlayerSkills Skills;
	int32 DaysOnStation = 0;
};
