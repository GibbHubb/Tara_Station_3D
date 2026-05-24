#include "UI/TaraHUDWidget.h"
#include "Components/TextBlock.h"
#include "Bridge/TaraSimSubsystem.h"
#include "Engine/GameInstance.h"

void UTaraHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Refresh();
}

void UTaraHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	SecondsSinceRefresh += InDeltaTime;
	if (SecondsSinceRefresh < 0.25f) return;
	SecondsSinceRefresh = 0.0f;
	Refresh();
}

void UTaraHUDWidget::Refresh()
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;
	UTaraSimSubsystem* Sim = GI->GetSubsystem<UTaraSimSubsystem>();
	if (!Sim) return;

	if (Text_DateTime)
	{
		Text_DateTime->SetText(FText::FromString(FString::Printf(
			TEXT("Y%d D%d  %s"),
			Sim->GetYear(), Sim->GetDayOfYear(), *Sim->GetFormattedTime())));
	}

	if (Text_Herd)
	{
		Text_Herd->SetText(FText::FromString(FString::Printf(
			TEXT("Herd %d  cond %d"),
			Sim->GetHerdSize(), FMath::RoundToInt32(Sim->GetHerdConditionMean()))));
	}

	if (Text_Paddock)
	{
		const float Grass = Sim->GetPaddockGrassKgPerHa(FocusedPaddockId);
		Text_Paddock->SetText(FText::FromString(FString::Printf(
			TEXT("%s: %d kg/ha"),
			*FocusedPaddockId, FMath::RoundToInt32(Grass))));
	}
}
