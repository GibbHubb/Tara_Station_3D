#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DataTable.h"
#include "SeasonClock.h"          // for ESeason — TaraSimCore is engine-agnostic; safe to include
#include "ShedActionMenu.generated.h"

class UTaraSimSubsystem;
class UVerticalBox;
class UTextBlock;
class UButton;

// ---- DataTable schema ---------------------------------------------------
//
// Per TS-3D-TWO-SEASON-MODE plan §6, `DT_ShedActions.uasset` lives in
// Content/UI/ and uses this row struct. Each row is one button on the shed
// action menu; the menu enables it based on the current ESeason vs the row's
// Season tag.
//
// BridgeFunctionName is the FName of a UFUNCTION(BlueprintCallable) on
// UTaraSimSubsystem (e.g. "SellCattle", "BuyWorkMachine"). The menu dispatches
// by reflection — FindFunctionByName + ProcessEvent — so the data table can
// add new actions without recompiling C++. Typo-safety trade: a warning fires
// (and the row is skipped) on a name miss.

// Parallel UENUM in TaraGame mirroring ESeason — needed because ESeason lives
// in TaraSimCore (Core-only, no UENUM macro available). Convert via the
// helpers below. "Any" means the action is enabled in both seasons (e.g.
// Take-a-Vehicle, the always-on daily-anchor exit per MAP_BRIEF §7).
UENUM(BlueprintType)
enum class EShedActionSeason : uint8
{
	Any = 0,
	Wet = 1,
	Dry = 2,
};

USTRUCT(BlueprintType)
struct TARAGAME_API FShedActionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ActionId;                            // e.g. "muster", "buy-grader"

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Label;                               // displayed on the button

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EShedActionSeason Season = EShedActionSeason::Any;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName BridgeFunctionName;                  // resolves via UTaraSimSubsystem::FindFunctionByName

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (MultiLine = "true"))
	FText DisabledTooltip;                     // shown when season-mismatched
};


// ---- UShedActionMenu ---------------------------------------------------
//
// Base class for the WBP_ShedActionMenu UMG widget. Data-driven layout: the
// widget reads `DT_ShedActions` on construct + spawns one button per row in
// the `ActionList` VerticalBox, then subscribes to `Subsystem->OnSeasonChanged`
// to re-evaluate which buttons are enabled each time the season flips.
//
// Step 6 of the plan — bankruptcy override — lives in RefreshActionStates(): if
// `Subsystem->IsBankrupted()` is true, the action list is hidden + a single
// "in administration" message is shown. The Take-a-Vehicle button stays enabled
// (per the plan's agreed-state notes default) so the player can still drive
// off to resolve the boss-NPC conversation.

UCLASS(Abstract, Blueprintable)
class TARAGAME_API UShedActionMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	// Assign in the WBP_ShedActionMenu Designer (Content/UI/WBP_ShedActionMenu).
	UPROPERTY(EditDefaultsOnly, Category = "Tara|Shed")
	TObjectPtr<UDataTable> ActionsTable = nullptr;

	// Reserved action ID for the always-on daily-anchor exit. Rows with this
	// ActionId bypass the bankruptcy override + season filter — the player can
	// always leave the shed via a vehicle.
	static const FName TakeVehicleActionId;

	// Reserved action ID exposed to the Blueprint Designer to hook "Close"
	// button click → RemoveFromParent.
	UFUNCTION(BlueprintCallable, Category = "Tara|Shed")
	void Close();

protected:
	//~ Begin UUserWidget interface.
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	//~ End UUserWidget interface.

	// BindWidget — populated from WBP_ShedActionMenu by the BindWidget meta.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> ActionList = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SeasonLabel = nullptr;

	// Optional — shown only when IsBankrupted() is true. BindWidgetOptional
	// so the widget compiles even if the Designer hasn't added it yet.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BankruptcyMessage = nullptr;

private:
	// Subscription handle for OnSeasonChanged.
	FDelegateHandle SeasonChangedHandle;

	// Build buttons from the DataTable. Called once in NativeConstruct.
	void BuildButtons();

	// Walk the spawned buttons and update IsEnabled + tooltip text based on
	// the current season + bankruptcy state. Called from NativeConstruct,
	// from BuildButtons, and from the OnSeasonChanged delegate.
	void RefreshActionStates();

	// Per-button click handler. Looks up the row by ActionId, then dispatches
	// to the bridge function by name via reflection.
	UFUNCTION()
	void HandleActionButtonClicked();

	// Cached row pointers for the buttons we spawned (parallel to ActionList
	// children). One entry per spawned button.
	TArray<TWeakObjectPtr<UButton>> SpawnedButtons;
	TArray<FShedActionRow> SpawnedRows;

	// Helper.
	UTaraSimSubsystem* GetSubsystem() const;
};

// Free functions — ESeason ↔ EShedActionSeason converter (TaraSimCore enum vs
// the BlueprintType wrapper here in TaraGame).
TARAGAME_API EShedActionSeason ToShedActionSeason(ESeason Season);
TARAGAME_API bool SeasonMatchesAction(ESeason Current, EShedActionSeason Action);

// Default action set — used when ActionsTable is null (no DT_ShedActions
// asset has been authored yet). Lets PIE work on day one with sensible
// rows; populating DT_ShedActions in the Editor overrides this. The
// CSV at Content/UI/DT_ShedActions.csv mirrors this list 1:1 for the
// initial Editor import.
TARAGAME_API TArray<FShedActionRow> ShedActionMenu_DefaultRows();
