#include "UI/TaraHUDWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Widget.h"
#include "Bridge/TaraSimSubsystem.h"
#include "Station.h"
#include "Entities/CattleCohort.h"
#include "Entities/Herd.h"
#include "Engine/GameInstance.h"

void UTaraHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Refresh();
}

void UTaraHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const float RefreshInterval = (RefreshHz > 0.0f) ? (1.0f / RefreshHz) : 0.25f;
	SecondsSinceRefresh += InDeltaTime;
	if (SecondsSinceRefresh < RefreshInterval) return;
	SecondsSinceRefresh = 0.0f;
	Refresh();
}

void UTaraHUDWidget::SetTextSafe(UTextBlock* Target, const FString& Value)
{
	if (Target) Target->SetText(FText::FromString(Value));
}

void UTaraHUDWidget::SetVisibleSafe(UWidget* Target, bool bVisible)
{
	if (Target) Target->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UTaraHUDWidget::Refresh()
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;
	UTaraSimSubsystem* Sim = GI->GetSubsystem<UTaraSimSubsystem>();
	if (!Sim) return;

	// ---- M1: date/time/season + herd + paddock ----
	SetTextSafe(Text_DateTime, FString::Printf(TEXT("Y%d D%d  %s"),
		Sim->GetYear(), Sim->GetDayOfYear(), *Sim->GetFormattedTime()));

	if (Text_Season)
	{
		const ESeason S = Sim->GetStation() ? Sim->GetStation()->GetClock().GetSeason() : ESeason::Wet;
		SetTextSafe(Text_Season, S == ESeason::Wet ? TEXT("Wet") : TEXT("Dry"));
	}

	SetTextSafe(Text_Herd, FString::Printf(TEXT("Herd %d  cond %d"),
		Sim->GetHerdSize(), FMath::RoundToInt32(Sim->GetHerdConditionMean())));

	if (Text_Paddock)
	{
		const float Grass = Sim->GetPaddockGrassKgPerHa(FocusedPaddockId);
		const int32 Water = Sim->GetPaddockWaterAccess(FocusedPaddockId);
		const TCHAR* WaterStr = (Water == 0) ? TEXT("Full") : (Water == 1) ? TEXT("Low") : TEXT("None");
		SetTextSafe(Text_Paddock, FString::Printf(TEXT("%s: %d kg/ha · water %s"),
			*FocusedPaddockId, FMath::RoundToInt32(Grass), WaterStr));
	}

	// ---- M3: economy ----
	SetTextSafe(Text_Cash, FString::Printf(TEXT("$%d"), Sim->GetPlayerCash()));
	SetTextSafe(Text_CattlePrice, FString::Printf(TEXT("Beasts @ $%d"), Sim->GetCurrentCattlePrice()));

	if (Text_Role)
	{
		const int32 R = Sim->GetPlayerRole();
		const TCHAR* RoleName = (R == 0) ? TEXT("Ringer") : (R == 1) ? TEXT("Manager") : TEXT("Owner");
		SetTextSafe(Text_Role, RoleName);
	}

	// ---- M4a: muster transit ----
	if (Text_MusterStatus)
	{
		if (Sim->IsHerdInTransit())
		{
			SetTextSafe(Text_MusterStatus, FString::Printf(TEXT("Mustering → %s (%dd left)"),
				*Sim->GetMusterDestinationPaddock(), Sim->GetMusterDaysRemaining()));
		}
		else
		{
			SetTextSafe(Text_MusterStatus, TEXT("Idle"));
		}
	}

	// ---- M5: drought + breeding ----
	const float D = Sim->GetCurrentDroughtSeverity();
	if (Bar_Drought)
	{
		Bar_Drought->SetPercent(D);
		// Show only when there's something to show.
		SetVisibleSafe(Bar_Drought, D > 0.0f);
	}
	if (Text_DroughtSeverity)
	{
		if (D > 0.0f)
		{
			SetTextSafe(Text_DroughtSeverity, FString::Printf(TEXT("Drought %.2f"), D));
			SetVisibleSafe(Text_DroughtSeverity, true);
		}
		else
		{
			SetVisibleSafe(Text_DroughtSeverity, false);
		}
	}

	if (Text_BreedingState)
	{
		const int32 Pregnant = Sim->GetPregnantHead();
		if (Pregnant > 0)
		{
			SetTextSafe(Text_BreedingState, FString::Printf(TEXT("Pregnant: %d head"), Pregnant));
			SetVisibleSafe(Text_BreedingState, true);
		}
		else
		{
			SetVisibleSafe(Text_BreedingState, false);
		}
	}

	// ---- M6: researcher ----
	SetTextSafe(Text_BirdLog, FString::Printf(TEXT("Birds: %d logged"), Sim->GetLoggedSpeciesCount()));

	// ---- M7: bankruptcy warning ----
	if (Text_BankruptcyWarning)
	{
		const int32 Days = Sim->GetDaysInBankruptcy();
		if (Days > 0)
		{
			const int32 DaysLeft = 60 - Days;   // matches FProgressionSystem::BankruptcyDebtDays
			SetTextSafe(Text_BankruptcyWarning, FString::Printf(
				TEXT("⚠ Bankruptcy in %dd"), FMath::Max(0, DaysLeft)));
			SetVisibleSafe(Text_BankruptcyWarning, true);
		}
		else
		{
			SetVisibleSafe(Text_BankruptcyWarning, false);
		}
	}

	// ---- M8: sensors (informational) ----
	if (Text_SensorCount)
	{
		const int32 Count = Sim->GetSensorCount();
		if (Count > 0)
		{
			SetTextSafe(Text_SensorCount, FString::Printf(TEXT("Sensors: %d"), Count));
			SetVisibleSafe(Text_SensorCount, true);
		}
		else
		{
			SetVisibleSafe(Text_SensorCount, false);
		}
	}

	// ---- M5: bad-weather decision pending alert ----
	if (Text_DecisionAlert)
	{
		if (Sim->HasPendingBadWeatherDecision())
		{
			SetTextSafe(Text_DecisionAlert, TEXT("⚠ Drought decision pending"));
			SetVisibleSafe(Text_DecisionAlert, true);
		}
		else
		{
			SetVisibleSafe(Text_DecisionAlert, false);
		}
	}
}
