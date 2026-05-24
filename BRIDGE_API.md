# UTaraSimSubsystem — Bridge API reference

> **For:** wiring UMG widgets, Actors, and BP graphs against the sim. Every public surface of `UTaraSimSubsystem` listed with semantics + example wire-ups.
> **As of:** 2026-05-25 (sim port 8/8 complete + pacing dials + TS-3D-TWO-SEASON-MODE C++ side at Tara_Station_3D `954b2ab`).
> **Companion to:** [README.md](README.md), [PHASE1_EDITOR_TODO.md](PHASE1_EDITOR_TODO.md), [PHASE2_SHED_EDITOR_TODO.md](PHASE2_SHED_EDITOR_TODO.md), and the planning docs in the 2D repo's [`docs/3d-pivot/`](https://github.com/GibbHubb/Tara_Station/tree/main/docs/3d-pivot).

The bridge has one job: **own one `FStation` and expose its full surface to Blueprints / Widgets / Actors without leaking sim internals.** Every action below is a `UFUNCTION` on the subsystem. Every event is a multicast delegate any Actor can `AddUObject` / `AddDynamic` on. The sim itself never knows the bridge exists.

To get the subsystem from any Blueprint or UMG widget:

```cpp
// C++:
UTaraSimSubsystem* Sub = GetGameInstance()->GetSubsystem<UTaraSimSubsystem>();

// Blueprint: Get Game Instance → Get Subsystem (Tara Sim Subsystem)
```

---

## Pacing — real-time advance (call this from your GameMode Tick)

| Function | Returns | Notes |
|---|---|---|
| `AdvanceRealTime(float DeltaSeconds)` | void | Engine ticks call this once per frame. Internally accumulates → in-game minutes via `FSeasonClock::RealSecondsPerInGameDay` (default 1800 = 30 min real per in-game day; CORE_LOOP §7 range 25-45 min). At sleep boundary (Hour ≥ 22:00) runs `Station::TickDay()`. |
| `TickDay()` | void | Force-advance one full sim day. Useful for debug (bind to a key during development) or when scripts need to jump time deterministically. |
| `SaveNow()` | void | Force-write `tara-save-3d-v8-m8.json`. Autosave fires automatically on every `DayEnded`. |
| `NewGame()` | void | Discard current `FStation` and re-seed defaults. |
| `GetSecondsPerInGameDay() : float` | 60..7200 | Current pacing dial. Default 1800. |
| `SetSecondsPerInGameDay(float)` | void | Settings-UI hook. Clamped 60..7200 (1 min .. 2 hr per in-game day). |
| `GetYearsToOwnerHypothesis() : int32` | 4 | Per CORE_LOOP §7. Tunable hypothesis only — used by UI hints, not enforced. |

**Day-tick pattern:**
```cpp
// BP_TaraGameMode::Tick (or any persistent Actor's Tick)
void ATaraGameMode::Tick(float Delta)
{
    Super::Tick(Delta);
    if (auto* Sub = GetGameInstance()->GetSubsystem<UTaraSimSubsystem>())
    {
        Sub->AdvanceRealTime(Delta);
    }
}
```

---

## M1 — herd condition + paddock grass

### Actions
*(none — M1 is read-only sim that ticks autonomously)*

### Pure getters
| Function | Returns | Notes |
|---|---|---|
| `GetYear()` | int32 | 1-based, increments at DayOfYear=365 |
| `GetDayOfYear()` | int32 | 1..365 |
| `GetFormattedTime()` | FString | `"HH:MM"`, e.g. `"14:30"` |
| `GetHerdSize()` | int32 | Sum of all cohort counts |
| `GetHerdConditionMean()` | float | 0..100, weighted by cohort size |
| `GetPaddockGrassKgPerHa(PaddockId)` | float | 0..3500 (clamped) |

### Events
| Delegate | Fires when | Payload fields |
|---|---|---|
| `OnDayEnded` | End of `Station::TickDay()` (after all systems) | `Day`, `Year` |
| `OnCattleDied` | Starvation or pest-kill takes head from a cohort | `HerdId`, `CohortBirthYear`, `Count`, `Cause` |
| `OnHerdConditionChanged` | ConditionSystem applies a non-zero delta | `HerdId`, `CohortBirthYear`, `Delta`, `Mean` |

**HUD wire-up:**
```cpp
// In WBP_TaraHUD::NativeConstruct, or via BindEvent in the Designer:
Sub->OnDayEnded.AddUObject(this, &UWBP_TaraHUD::HandleDayEnded);
// Per-frame just read:
DateText->SetText(FText::FromString(FString::Printf(
    TEXT("Y%d D%d %s"),
    Sub->GetYear(), Sub->GetDayOfYear(), *Sub->GetFormattedTime())));
HerdText->SetText(FText::AsNumber(Sub->GetHerdSize()));
```

---

## M2 — water + bores

### Actions
| Function | Returns | Notes |
|---|---|---|
| `CheckBore(BoreId)` | bool | Tries to repair a bore. 80% success roll; logs days spent. Returns true on fix. |

### Pure getters
| Function | Returns | Notes |
|---|---|---|
| `GetPaddockWaterAccess(PaddockId)` | int32 | 0=Full / 1=Low / 2=None |

### Events
| Delegate | Fires when | Payload |
|---|---|---|
| `OnBoreFailed` | Bore condition daily-roll fails | `BoreId`, `PaddockIds[]` (paddocks served) |
| `OnBoreRepaired` | `CheckBore` succeeds | `BoreId` |
| `OnWaterAccessLost` | Paddock loses all water (last serving bore failed) | `PaddockId` |
| `OnWaterAccessRestored` | Paddock regains water (any serving bore back online) | `PaddockId` |

---

## M3 — economy

### Actions
| Function | Returns | Notes |
|---|---|---|
| `BuySupplement(int32 Kind)` | bool | Kind: 0=Lick, 1=Molasses, 2=Feed. Charges player; queues delivery. Returns false if waterless paddock or insufficient funds. |
| `SellCattle(int32 Count)` | int32 | Returns $ earned (0 on fail). Pulls from largest cohort first. Price = base × seasonal × drought modifiers. |

### Pure getters
| Function | Returns | Notes |
|---|---|---|
| `GetPlayerCash()` | int32 | Can go negative (bankruptcy timer starts) |
| `GetPlayerRole()` | int32 | 0=Ringer, 1=Manager, 2=Owner |
| `GetCurrentCattlePrice()` | int32 | Per-head, season + drought adjusted |

### Events
| Delegate | Fires when | Payload |
|---|---|---|
| `OnMoneyChanged` | Any cash debit/credit | `Delta`, `Reason`, `NewCash` |
| `OnCattleSold` | `SellCattle` succeeds | `Count`, `PricePerHead`, `Total` |
| `OnSupplementOrdered` | `BuySupplement` succeeds | `Kind`, `Cost`, `ArrivesInDays` |
| `OnYearEnded` | DayOfYear rolls 365→1 | `Year` |

---

## M4a — mustering + movement + fences

### Actions
| Function | Returns | Notes |
|---|---|---|
| `StartMuster(ToPaddockId, VehicleType, HandIds[], PlannedDays, FuelCost, BreakawayPending)` | bool | **3D layer computes args spatially**, then commits. Sim runs the clock + applies breakaway at completion. |
| `CancelMuster()` | bool | Refunds half fuel; emits MusterCancelled |
| `BuyVehicle(int32 Type)` | bool | 0=Foot, 1=Horse, 2=TwoWheeler, 3=FourWheeler, 4=Buggy, 5=Chopper |
| `HireHand(HandId)` | bool | False if already hired or unknown |
| `FireHand(HandId)` | bool | False if not hired |
| `RepairFence(FenceId)` | bool | Sets integrity to 100; emits FenceRepaired |

### Pure getters
| Function | Returns | Notes |
|---|---|---|
| `IsHerdInTransit()` | bool | True during a muster |
| `GetMusterDaysRemaining()` | int32 | 0 when idle |
| `GetMusterDestinationPaddock()` | FString | "" when idle |
| `IsVehicleOwned(int32 Type)` | bool | Foot + Horse always true (free) |
| `GetVehicleCondition(int32 Type)` | float | 0..100 |
| `GetFenceIntegrity(FenceId)` | float | 0..100; 0 = broken, cattle drift |
| `GetRoadGradeQuality(RoadId)` | float | 0..100; below 60 penalises wheeled vehicles |
| `IsHandHired(HandId)` | bool | |

### Events
| Delegate | Fires when | Payload |
|---|---|---|
| `OnMusterStarted` | `StartMuster` succeeds | `FromPaddockId`, `ToPaddockId`, `VehicleType`, `HandIds[]`, `PlannedDays`, `FuelCost`, `BreakawayPending` |
| `OnMusterCompleted` | Muster ticks to 0 days | `ToPaddockId`, `VehicleType`, `HandIds[]`, `BreakawayHead` (actual) |
| `OnMusterCancelled` | `CancelMuster` succeeds | `ToPaddockId`, `DaysSpent`, `Refund` |
| `OnBreakaway` | Cohort loses head during a muster | `Count`, `PaddockId` |
| `OnFenceBroken` | Fence integrity crosses 0 | `FenceId`, `PaddockA`, `PaddockB` |
| `OnCattleDrifted` | Broken-fence drift (1-5 head/day) | `FromPaddockId`, `ToPaddockId`, `Count` |
| `OnVehiclePurchased` | `BuyVehicle` succeeds | `VehicleType`, `Cost` |
| `OnHandHired` / `OnHandFired` | Hire/fire action | `HandId` |

**Spatial muster wire-up pattern (Phase 2+):**
```cpp
// When player completes the spatial muster (arrives at destination with the mob):
const FMusterArgs Args = ComputeMusterFromSpatialState(); // 3D layer
Sub->StartMuster(Args.ToPaddockId, Args.VehicleType, Args.HandIds,
                 Args.PlannedDays, Args.FuelCost, Args.BreakawayPending);
// Sim ticks the clock; OnMusterCompleted fires at completion.
```

---

## M5 — events + breeding

### Actions
| Function | Returns | Notes |
|---|---|---|
| `ResolveBadWeatherDecision(FString Choice)` | void | Choice: `"feed"` / `"agist"` / `"sell"` / `"hold"`. Closes the drought modal. |

### Pure getters
| Function | Returns | Notes |
|---|---|---|
| `GetCurrentDroughtSeverity()` | float | 0..1; writes to PriceTable.DroughtSeverity each tick |
| `IsFlooding()` | bool | Any active flood event |
| `GetGrassGrowthMultiplier()` | float | 0..1; drought cuts up to 70% |
| `HasPendingBadWeatherDecision()` | bool | True between drought start + player choice |
| `GetPregnantHead()` | int32 | Sum of all active pregnancy records |

### Events
| Delegate | Fires when | Payload |
|---|---|---|
| `OnEventStarted` | Random event fires | `Type` (int), `Severity` (0..1), `DurationDays`, `EventId` |
| `OnEventResolved` | Event duration ends | `Type`, `EventId`, `Outcome` |
| `OnBadWeatherDecisionRequired` | Drought start | `EventId` |
| `OnBadWeatherDecisionMade` | `ResolveBadWeatherDecision` called | `EventId`, `Choice` |
| `OnCalendarEventDue` | Rodeo (day 120) or Draft (day 220) | `Type` (`"rodeo"` / `"draft"`) |
| `OnBreedingWindowOpened` | Day 75 each year | `CohortBirthYear` (the breeders' cohort) |
| `OnBreedingConceived` | Window close + condition ≥55 | `CohortBirthYear`, `PregnantCount` |
| `OnCalfBorn` | Gestation completes (280d) | `CohortBirthYear` (the new calf cohort), `CalfCount` |

**Drought visual reaction wire-up:**
```cpp
// In APaddockActor::Tick or via OnEventStarted:
const float D = Sub->GetCurrentDroughtSeverity();
GrassMaterial->SetScalarParameterValue("BleachAmount", D);
GrassMaterial->SetScalarParameterValue("GrowthMultiplier", Sub->GetGrassGrowthMultiplier());
```

---

## M6 — wildlife + invasives

### Actions
| Function | Returns | Notes |
|---|---|---|
| `LogBird(SpeciesId)` | int32 | Returns researcher payout (0 if not first log of this species). Cash credited automatically. |
| `ShootPest(PaddockId, Species)` | bool | Species: `"dingo"` / `"feral-cat"` / `"wild-pig"`. Skill-gated. |
| `TreatInvasive(PaddockId, Species)` | bool | Species: `"parkinsonia"` / `"rubber-vine"` / `"prickly-acacia"`. Knocks coverage -25. |

### Pure getters
| Function | Returns | Notes |
|---|---|---|
| `GetLoggedSpeciesCount()` | int32 | Used in M7 year-end score (10 logs = max 100/100 bird score) |
| `GetPestPopulation(PaddockId, Species)` | float | 0 if none present |
| `GetInvasiveCoverage(PaddockId, Species)` | float | 0..100 % |

### Events
| Delegate | Fires when | Payload |
|---|---|---|
| `OnBirdSighted` | (Reserved — not yet auto-fired; spatial trigger when bird actor enters player range) | `SpeciesId`, `Name` |
| `OnBirdLogged` | `LogBird` called | `SpeciesId`, `Payout`, `bFirst` |
| `OnPestShot` | `ShootPest` succeeds | `PaddockId`, `Species` (int) |
| `OnInvasiveTreated` | `TreatInvasive` succeeds | `PaddockId`, `Species` (int), `NewCoverage` |
| `OnInvasiveSpread` | Invasive jumps to adjacent paddock (>80% coverage roll) | `FromPaddockId`, `ToPaddockId`, `Species` |

---

## M7 — progression

### Actions
| Function | Returns | Notes |
|---|---|---|
| `BuyProperty()` | bool | Manager→Owner; $250k threshold; false if wrong role or insufficient funds |
| `EvaluateYearNow()` | float | Returns the 0-100 score. Triggers role transition (Ringer→Manager at ≥70). Year-end UI normally calls this on `OnYearEnded`. |

### Pure getters
| Function | Returns | Notes |
|---|---|---|
| `CanBuyProperty()` | bool | True iff Role==Manager and Cash ≥ $250k |
| `GetDaysInBankruptcy()` | int32 | Counts up while Owner with Cash<0 |
| `IsBankrupted()` | bool | True at 60+ days of bankruptcy |

### Events
| Delegate | Fires when | Payload |
|---|---|---|
| `OnRoleChanged` | Auto-promotion or `BuyProperty` | `From` (int), `To` (int) |
| `OnYearEvaluated` | `EvaluateYearNow` called | `Year`, `Score`, `NewRole`, `bPromoted`, `bFired` |
| `OnPropertyPurchased` | `BuyProperty` succeeds | `Cost` |
| `OnBankruptcyDeclared` | Day 60 of negative cash | `Year`, `DaysInDebt` |

---

## M8 — machinery + sensors

### Actions
| Function | Returns | Notes |
|---|---|---|
| `BuyWorkMachine(int32 Type)` | bool | 0=Tractor, 1=FrontEndLoader, 2=Forklift, 3=Digger, 4=Grader, 5=FlatBedTrailer, 6=MolassesTruck. $8k-$95k. |
| `InstallSensor(Kind, LocationId)` | bool | Kind: `"rain-gauge"` (350) / `"tank-flow"` (600) / `"bore-pressure"` (800). |
| `SwapSensorBattery(SensorId)` | bool | Resets 180-day battery |
| `GradeRoad(RoadId)` | bool | +50 grade quality; emits RoadGraded |

### Pure getters
| Function | Returns | Notes |
|---|---|---|
| `IsWorkMachineOwned(int32 Type)` | bool | All 7 unowned at game start |
| `GetSensorCount()` | int32 | Total installed sensors |
| `GetSensorReading(SensorId)` | FString | Display string (e.g. `"7.2 mm"` / `"82% full"` / `"cond 65%"`) |
| `GetSensorBatteryDays(SensorId)` | int32 | 0 = flat |

### Events
| Delegate | Fires when | Payload |
|---|---|---|
| `OnWorkMachinePurchased` | `BuyWorkMachine` succeeds | `Type`, `Cost` |
| `OnSensorInstalled` | `InstallSensor` succeeds | `SensorId`, `Kind`, `LocationId` |
| `OnSensorBatterySwapped` | `SwapSensorBattery` succeeds | `SensorId` |
| `OnFenceRepaired` | `RepairFence` succeeds | `FenceId` |
| `OnRoadGraded` | `GradeRoad` succeeds | `RoadId` |

---

## Bridge-synthesised events

These don't exist on `FEventBus` — they're synthesised in `UTaraSimSubsystem` from sim state changes that don't have a first-class event. Cheap to add when a UI concern needs reactivity but the sim doesn't.

| Delegate | Fires when | Payload | Synthesis source |
|---|---|---|---|
| `OnSeasonChanged` | DayEnded reveals `Clock.GetSeason()` changed since last tick | `From` (ESeason), `To` (ESeason) | `OnDayEnded` handler compares pre/post |

**Pattern:** if a UI element needs to react to something the sim doesn't formally announce, add a bridge-synth delegate rather than extending `FEventBus`. Keeps the sim layer minimal.

---

## Performance notes

- All bridge functions are cheap (sim state is in-memory; reads are O(1) or O(small-N)).
- Delegates broadcast synchronously inside `TickDay` — handlers shouldn't do heavy work (no level loads, no async file I/O). Use the delegate to flip a flag + react on the next engine Tick if needed.
- `WriteSaveToDisk` (autosave on DayEnded) writes a few KB of JSON. Negligible at the once-per-day cadence; would matter if you ever wired autosave to AdvanceRealTime.

---

## When in doubt

The sim source is human-readable and short. If a behaviour is unclear, read:
- `Source/TaraSimCore/Public/Station.h` — TickDay sequence is the canonical ordering
- `Source/TaraSimCore/Public/Systems/<Name>System.h` — each system's contract
- `Source/TaraSimCore/Public/EventPayloads.h` — every event's exact fields

The bridge (`Source/TaraGame/Public/Bridge/TaraSimSubsystem.h` + `.cpp`) is the only file that connects these to engine concepts.
