#pragma once

#include "CoreMinimal.h"

// Mirrors src/sim/entities/Trough.ts. Placeholder type — the trough is the
// cattle-facing end of a bore. M2 keeps it as a thin concept; M8 deepens
// (per-trough capacity, fouling, separate maintenance from bores).
//
// In 3D it gains a real placed Actor with visible water-level state, but the
// sim still doesn't need much here.

struct TARASIMCORE_API FTrough
{
	FString Id;
	FString BoreId;
	FString PaddockId;
};
