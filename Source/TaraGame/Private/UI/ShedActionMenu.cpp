#include "UI/ShedActionMenu.h"
#include "Bridge/TaraSimSubsystem.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Widget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"

const FName UShedActionMenu::TakeVehicleActionId = TEXT("take-vehicle");

EShedActionSeason ToShedActionSeason(ESeason Season)
{
	switch (Season)
	{
	case ESeason::Wet: return EShedActionSeason::Wet;
	case ESeason::Dry: return EShedActionSeason::Dry;
	default: return EShedActionSeason::Any;
	}
}

bool SeasonMatchesAction(ESeason Current, EShedActionSeason Action)
{
	if (Action == EShedActionSeason::Any) return true;
	return ToShedActionSeason(Current) == Action;
}

UTaraSimSubsystem* UShedActionMenu::GetSubsystem() const
{
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(GetWorld()))
	{
		return GI->GetSubsystem<UTaraSimSubsystem>();
	}
	return nullptr;
}

void UShedActionMenu::NativeConstruct()
{
	Super::NativeConstruct();

	BuildButtons();
	RefreshActionStates();

	// Subscribe to OnSeasonChanged so the menu re-evaluates if the player
	// leaves it open across a day-tick that crosses the season boundary.
	if (UTaraSimSubsystem* Sub = GetSubsystem())
	{
		SeasonChangedHandle = Sub->OnSeasonChanged.AddWeakLambda(this,
			[this](ESeason /*From*/, ESeason /*To*/)
			{
				RefreshActionStates();
			});
	}
}

void UShedActionMenu::NativeDestruct()
{
	if (UTaraSimSubsystem* Sub = GetSubsystem())
	{
		if (SeasonChangedHandle.IsValid())
		{
			Sub->OnSeasonChanged.Remove(SeasonChangedHandle);
			SeasonChangedHandle.Reset();
		}
	}
	Super::NativeDestruct();
}

void UShedActionMenu::Close()
{
	RemoveFromParent();
}

void UShedActionMenu::BuildButtons()
{
	if (!ActionList || !ActionsTable) return;

	ActionList->ClearChildren();
	SpawnedButtons.Reset();
	SpawnedRows.Reset();

	TArray<FShedActionRow*> Rows;
	ActionsTable->GetAllRows<FShedActionRow>(TEXT("ShedActionMenu::BuildButtons"), Rows);
	for (FShedActionRow* RowPtr : Rows)
	{
		if (!RowPtr) continue;
		const FShedActionRow& Row = *RowPtr;

		UButton* Btn = NewObject<UButton>(this);
		if (!Btn) continue;

		// Label child — a TextBlock inside the button.
		UTextBlock* LabelText = NewObject<UTextBlock>(Btn);
		if (LabelText)
		{
			LabelText->SetText(Row.Label.IsEmpty()
				? FText::FromName(Row.ActionId)
				: Row.Label);
			Btn->AddChild(LabelText);
		}

		// Wire OnClicked → HandleActionButtonClicked.
		Btn->OnClicked.AddDynamic(this, &UShedActionMenu::HandleActionButtonClicked);
		ActionList->AddChildToVerticalBox(Btn);

		SpawnedButtons.Add(Btn);
		SpawnedRows.Add(Row);
	}
}

void UShedActionMenu::RefreshActionStates()
{
	UTaraSimSubsystem* Sub = GetSubsystem();
	if (!Sub)
	{
		// No subsystem yet (very early init or PIE quirk) — disable everything
		// to be safe; the next OnSeasonChanged or re-construct will fix it.
		for (TWeakObjectPtr<UButton>& Wp : SpawnedButtons)
		{
			if (UButton* B = Wp.Get()) B->SetIsEnabled(false);
		}
		return;
	}

	// --- Bankruptcy override (plan step 6) ---
	// If the property is in administration, hide the entire action list and
	// show the bankruptcy message. Per plan agreed-state defaults: still
	// allow Take-a-Vehicle so the player can drive to the boss-NPC.
	const bool bBankrupt = Sub->IsBankrupted();
	if (BankruptcyMessage)
	{
		BankruptcyMessage->SetVisibility(bBankrupt
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
	}

	const ESeason Current = Sub->GetStation()
		? Sub->GetStation()->GetClock().GetSeason()
		: ESeason::Wet;

	if (SeasonLabel)
	{
		SeasonLabel->SetText(FText::FromString(
			Current == ESeason::Wet
				? TEXT("Wet season — build mode")
				: TEXT("Dry season — cattle work")));
	}

	for (int32 I = 0; I < SpawnedButtons.Num() && I < SpawnedRows.Num(); I++)
	{
		UButton* B = SpawnedButtons[I].Get();
		if (!B) continue;

		const FShedActionRow& Row = SpawnedRows[I];
		const bool bIsTakeVehicle = (Row.ActionId == TakeVehicleActionId);

		// Take-a-Vehicle bypasses bankruptcy + season filter.
		if (bIsTakeVehicle)
		{
			B->SetIsEnabled(true);
			B->SetVisibility(ESlateVisibility::Visible);
			B->SetToolTipText(FText::GetEmpty());
			continue;
		}

		// Bankruptcy override: hide all non-vehicle actions entirely.
		if (bBankrupt)
		{
			B->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}

		// Normal season filter — grey out (not hide) so the player SEES the
		// rhythm. Per plan §5: "grey-out, not hide" is the explicit design
		// call — half the buttons go dim in October, the other half in May.
		const bool bMatches = SeasonMatchesAction(Current, Row.Season);
		B->SetVisibility(ESlateVisibility::Visible);
		B->SetIsEnabled(bMatches);
		B->SetToolTipText(bMatches
			? FText::GetEmpty()
			: Row.DisabledTooltip);
	}
}

void UShedActionMenu::HandleActionButtonClicked()
{
	UTaraSimSubsystem* Sub = GetSubsystem();
	if (!Sub) return;

	// Identify which button fired. UButton::OnClicked doesn't pass the sender
	// in AddDynamic, so we walk the children and check IsHovered as a proxy.
	// Fallback: the FIRST currently-enabled button under the cursor wins —
	// good enough since the user can only physically click one.
	int32 ClickedIdx = INDEX_NONE;
	for (int32 I = 0; I < SpawnedButtons.Num(); I++)
	{
		UButton* B = SpawnedButtons[I].Get();
		if (B && B->IsHovered() && B->GetIsEnabled())
		{
			ClickedIdx = I;
			break;
		}
	}
	if (ClickedIdx == INDEX_NONE || ClickedIdx >= SpawnedRows.Num()) return;

	const FShedActionRow& Row = SpawnedRows[ClickedIdx];

	// Take-a-Vehicle is a UI-only action — the Blueprint subclass overrides
	// HandleActionButtonClicked or wires up its own logic to pop the
	// vehicle-pick widget. The C++ base only handles bridge-call rows.
	if (Row.ActionId == TakeVehicleActionId)
	{
		// Concrete BP can override; default no-op.
		return;
	}

	if (Row.BridgeFunctionName.IsNone())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ShedActionMenu] row '%s' has no BridgeFunctionName — skipping"),
			*Row.ActionId.ToString());
		return;
	}

	UFunction* Func = Sub->FindFunctionByName(Row.BridgeFunctionName);
	if (!Func)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ShedActionMenu] bridge function '%s' not found on UTaraSimSubsystem (row '%s')"),
			*Row.BridgeFunctionName.ToString(), *Row.ActionId.ToString());
		return;
	}

	// Parameterless bridge functions are the simplest case (e.g. BuyProperty,
	// SaveNow, TickDay). Parameterised ones (SellCattle(count), BuySupplement
	// (kind), BuyWorkMachine(type), etc.) need the Blueprint subclass to pop a
	// secondary widget that gathers the args + calls the bridge directly. We
	// dispatch only the no-args variant from C++; richer rows route through
	// BP. Document this in DT_ShedActions tooltips.
	if (Func->NumParms == 0 ||
		(Func->NumParms == 1 && Func->GetReturnProperty() != nullptr))
	{
		// 0 args (e.g. BuyProperty/SaveNow) OR 0 args with a single return (e.g.
		// EvaluateYearNow returns float). ProcessEvent expects a buffer
		// matching the full parm layout including return.
		uint8 ReturnBuffer[64] = {0};
		Sub->ProcessEvent(Func, ReturnBuffer);
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ShedActionMenu] bridge function '%s' takes parameters — route through a BP-side handler instead of the no-args dispatcher"),
			*Row.BridgeFunctionName.ToString());
	}
}
