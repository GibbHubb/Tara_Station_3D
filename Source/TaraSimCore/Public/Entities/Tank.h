#pragma once

#include "CoreMinimal.h"

// Mirrors src/sim/entities/Tank.ts. Placeholder for M8 (capacity, fill from
// windmills/bores, rain-gauge sensor integration). Logic shape carries over
// unchanged.

enum class ETankStatus : uint8
{
	Working,
	Leaking,
	Empty,
};

struct TARASIMCORE_API FTank
{
	FString Id;
	FString PaddockId;
	int32 CapacityLitres = 50000;
	int32 CurrentLitres = 50000;
	ETankStatus Status = ETankStatus::Working;
};
