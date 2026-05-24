#pragma once

#include "CoreMinimal.h"

// Mirrors src/sim/entities/Bore.ts. A bore is a water point with explicit
// `ServesPaddockIds` (length 1 or 2). The 2-paddock case is the "shared bore
// between adjacent paddocks" — when it fails, both paddocks lose water at
// once. The 1-paddock case is the simpler topology.
//
// In 3D, ABoreActor in TaraGame holds a non-owning pointer to FBore and
// renders the windmill + tank + trough silhouette at its placed transform.

enum class EBoreStatus : uint8
{
	Working,
	Failed,
};

struct FBoreConfig
{
	FString Id;
	TArray<FString> ServesPaddockIds;  // length 1 or 2; if 2, paddocks must be adjacent
	float Condition = 90.0f;
	EBoreStatus Status = EBoreStatus::Working;
};

class TARASIMCORE_API FBore
{
public:
	explicit FBore(const FBoreConfig& Config);

	static float ClampCondition(float V);

	FString SerializeJson() const;
	static FBore FromJson(const FString& Json);

public:
	FString Id;
	TArray<FString> ServesPaddockIds;
	float Condition = 90.0f;
	EBoreStatus Status = EBoreStatus::Working;
	int32 LastCheckedDay = -1;
};
