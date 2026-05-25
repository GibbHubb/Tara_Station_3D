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

TArray<FShedActionRow> ShedActionMenu_DefaultRows()
{
	auto MakeRow = [](const TCHAR* Id, const TCHAR* Label, EShedActionSeason Season,
		const TCHAR* BridgeFn, const TCHAR* DisabledTip) -> FShedActionRow
	{
		FShedActionRow R;
		R.ActionId = FName(Id);
		R.Label = FText::FromString(Label);
		R.Season = Season;
		R.BridgeFunctionName = FName(BridgeFn);
		R.DisabledTooltip = FText::FromString(DisabledTip);
		return R;
	};

	// Default roster per the TS-3D-TWO-SEASON-MODE plan §3 acceptance. Order
	// here drives display order in the VerticalBox. Tooltip text quotes the
	// real station-work language Max wants surfaced (CORE_LOOP §3).
	return {
		// Always-on daily-anchor exit (per MAP_BRIEF §7 + plan §3).
		MakeRow(TEXT("take-vehicle"), TEXT("Take a vehicle"),
			EShedActionSeason::Any, TEXT(""),
			TEXT("")),

		// --- Dry-season (cattle-work) actions ---
		MakeRow(TEXT("muster"), TEXT("Start a muster"),
			EShedActionSeason::Dry, TEXT(""),  // parameterised — handled by BP override
			TEXT("Too muddy to muster — wait for the dry.")),
		MakeRow(TEXT("yard-work"), TEXT("Yard work / drafting"),
			EShedActionSeason::Dry, TEXT(""),
			TEXT("Stock work waits for firm ground — wait for the dry.")),
		MakeRow(TEXT("order-supplement"), TEXT("Order supplement"),
			EShedActionSeason::Dry, TEXT(""),  // BuySupplement(kind) — BP override
			TEXT("Wet grass means no supplement needed yet — wait for the dry.")),
		MakeRow(TEXT("sell-cattle"), TEXT("Load sale cattle"),
			EShedActionSeason::Dry, TEXT(""),  // SellCattle(count) — BP override
			TEXT("Road trains can't get through in the wet — wait for the dry.")),
		MakeRow(TEXT("check-bore"), TEXT("Bore run"),
			EShedActionSeason::Dry, TEXT(""),  // CheckBore(id) — BP override
			TEXT("Bores top up from the wet — focus on capital work now.")),

		// --- Wet-season (capital-work) actions ---
		MakeRow(TEXT("repair-fence"), TEXT("Repair a fence"),
			EShedActionSeason::Wet, TEXT(""),  // RepairFence(id) — BP override
			TEXT("Fences hold through the dry — save repairs for the wet.")),
		MakeRow(TEXT("grade-road"), TEXT("Grade a road"),
			EShedActionSeason::Wet, TEXT(""),  // GradeRoad(id) — BP override
			TEXT("Save grading for the wet — easier on the machine.")),
		MakeRow(TEXT("buy-machine"), TEXT("Buy a work machine"),
			EShedActionSeason::Wet, TEXT(""),  // BuyWorkMachine(type) — BP override
			TEXT("Capital purchases are wet-season work.")),
		MakeRow(TEXT("install-sensor"), TEXT("Install a sensor"),
			EShedActionSeason::Wet, TEXT(""),  // InstallSensor(kind, loc) — BP override
			TEXT("Capital work — best in the wet.")),
		MakeRow(TEXT("bush-mechanic"), TEXT("Bush-mechanic the fleet"),
			EShedActionSeason::Wet, TEXT(""),
			TEXT("Save fleet maintenance for the wet.")),
	};
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
	if (!ActionList) return;

	ActionList->ClearChildren();
	SpawnedButtons.Reset();
	SpawnedRows.Reset();

	// Resolve row source: DT_ShedActions if assigned, else the C++ defaults so
	// PIE works on day one without the Editor authoring step. Authoring the
	// DT_ShedActions asset (see PHASE2_SHED_EDITOR_TODO.md) overrides this.
	TArray<FShedActionRow> Rows;
	if (ActionsTable)
	{
		TArray<FShedActionRow*> RowPtrs;
		ActionsTable->GetAllRows<FShedActionRow>(TEXT("ShedActionMenu::BuildButtons"), RowPtrs);
		for (FShedActionRow* RowPtr : RowPtrs)
		{
			if (RowPtr) Rows.Add(*RowPtr);
		}
	}
	if (Rows.Num() == 0)
	{
		Rows = ShedActionMenu_DefaultRows();
		UE_LOG(LogTemp, Display,
			TEXT("[ShedActionMenu] using %d default rows (ActionsTable not assigned)"),
			Rows.Num());
	}

	for (const FShedActionRow& Row : Rows)
	{

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

// Note: Rows-by-value loop above means we copy each row into SpawnedRows.
// That's fine — rows are tiny POD-ish structs; copying simplifies lifetime
// across the DT/default split.


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

	UFunction* Func = Sub->GetClass()->FindFunctionByName(Row.BridgeFunctionName);
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
