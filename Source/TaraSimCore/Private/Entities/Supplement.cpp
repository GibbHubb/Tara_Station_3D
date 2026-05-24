#include "Entities/Supplement.h"

float FSupplementProfile::GrassFloorLift(ESupplementKind Kind)
{
	switch (Kind)
	{
	case ESupplementKind::Lick:     return 100.0f;
	case ESupplementKind::Molasses: return 250.0f;
	case ESupplementKind::Feed:     return 600.0f;
	}
	return 0.0f;
}

float FSupplementProfile::ConditionRecoveryBoost(ESupplementKind Kind)
{
	switch (Kind)
	{
	case ESupplementKind::Lick:     return 0.0f;
	case ESupplementKind::Molasses: return 0.2f;
	case ESupplementKind::Feed:     return 0.6f;
	}
	return 0.0f;
}

int32 FSupplementProfile::DefaultDurationDays(ESupplementKind Kind)
{
	switch (Kind)
	{
	case ESupplementKind::Lick:     return 14;
	case ESupplementKind::Molasses: return 10;
	case ESupplementKind::Feed:     return 7;
	}
	return 0;
}

int32 FSupplementProfile::DefaultDeliveryDays(ESupplementKind Kind)
{
	switch (Kind)
	{
	case ESupplementKind::Lick:     return 1;
	case ESupplementKind::Molasses: return 2;
	case ESupplementKind::Feed:     return 1;
	}
	return 0;
}
