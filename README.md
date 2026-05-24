# Tara_Station_3D — Outback Life (Unreal Engine 5)

A 3D cattle-station life-sim built on the simulation already developed across M1–M8 of the 2D Phaser/TypeScript prototype ([`GibbHubb/Tara_Station`](https://github.com/GibbHubb/Tara_Station)). The 2D build is the **design reference**; this is the publishable 3D rebuild.

> **Status (2026-05-24): ✦ Sim port COMPLETE — 8/8 milestones at parity** with the 2D polish-pass build. M1 condition + M2 water + M3 economy + M4a mustering + M5 events/breeding + M6 wildlife + M7 progression + M8 machinery/sensors all live in `TaraSimCore` (pure C++, zero engine imports). Save schema at `v8-m8`. Bridge surfaces ~22 BlueprintCallable actions + ~30 BlueprintPure getters + 40+ multicast delegates. **Remaining work is all Editor-side** — see [`PHASE1_EDITOR_TODO.md`](PHASE1_EDITOR_TODO.md) for the vertical-slice setup.

See the 2D repo's [`3D_PROJECT_BRIEF.md`](https://github.com/GibbHubb/Tara_Station/blob/main/3D_PROJECT_BRIEF.md) for the single source of truth on direction, [`MAP_BRIEF.md`](https://github.com/GibbHubb/Tara_Station/blob/main/docs/3d-pivot/MAP_BRIEF.md) for the real-Tara spatial grounding, [`CORE_LOOP.md`](https://github.com/GibbHubb/Tara_Station/blob/main/docs/3d-pivot/CORE_LOOP.md) for the daily/yearly rhythm, and [`docs/3d-pivot/`](https://github.com/GibbHubb/Tara_Station/tree/main/docs/3d-pivot) for the sim-port audit, milestone plan, UE5 architecture, landmarks rulebook, and watershed design.

---

## Locked decisions (2026-05-24)

- **UE5 5.4+** (newest stable) — World Partition tooling needs 5.4+ for the continuous streamed world.
- **Repo is private** — flipped before first asset commits.
- **Real-Tara reference is the spatial source of truth** — actual Tara Pastoral Lease topographic map (1:17,500 GDA94/MGA54). Landmarks anchor on real terrain (monolith East, spires Limestone NE, gorge NW range near Granada, river along real central watercourse).
- **Cultural-sensitivity review** flagged for Phase 4 lead time on the monolith design (not engaging yet; 4-8 weeks ahead of any Phase 4 calendar slot).
- **Day length** TUNABLE — starting hypothesis 30 min/day (CORE_LOOP §7 range 25-45 min). Set via `Subsystem->SetSecondsPerInGameDay(seconds)`.
- **Ownership arc** TUNABLE — ~4 in-game years starting hypothesis (≈1 ringer / ~2 manager / then owner).

## Prerequisites

- **Unreal Engine 5.4+** (locked — open `Tara_Station_3D.uproject` and check the `EngineAssociation` field).
- **Visual Studio 2022** with the **Game development with C++** workload (`MSVC v143`, Windows 10/11 SDK).
- Git.

(Rider works as an alternative to Visual Studio. macOS Xcode toolchain also fine.)

## First-time setup

```bash
# 1. Right-click Tara_Station_3D.uproject → Generate Visual Studio project files
#    (or use the UnrealBuildTool / Engine helper script — varies by host OS)

# 2. Open the .sln in Visual Studio.

# 3. Build "Tara_Station_3DEditor — Development Editor" once.
#    First build compiles Engine + TaraSimCore + TaraGame; subsequent builds are fast.

# 4. Double-click Tara_Station_3D.uproject — opens the Editor.

# 5. Follow PHASE1_EDITOR_TODO.md to set up the level, materials, and the
#    cottage→shed→quad→paddock→bore→return Phase 1 vertical slice.
```

## Project layout

```
Tara_Station_3D/
├── Tara_Station_3D.uproject          ← project descriptor (UE5.4+)
├── Source/
│   ├── Tara_Station_3D.Target.cs     ← runtime target
│   ├── Tara_Station_3DEditor.Target.cs ← editor target
│   ├── TaraSimCore/                  ← engine-agnostic simulation (NO ENGINE)
│   │   ├── TaraSimCore.Build.cs      ← declares ONLY "Core" — CI enforced
│   │   ├── Public/
│   │   │   ├── EventBus.h            ← typed pub/sub (TFunction-based)
│   │   │   ├── EventPayloads.h       ← struct-per-event-type (~30 payloads)
│   │   │   ├── SeasonClock.h         ← year/day/hour/minute + season + tunable
│   │   │   │                            real-time advance (CORE_LOOP §7 pacing)
│   │   │   ├── Station.h             ← orchestrator (owns all entities + systems)
│   │   │   ├── Entities/
│   │   │   │   ├── Paddock.h         ← M1 — area + grass + intake/growth math
│   │   │   │   ├── CattleCohort.h    ← M1 — birth-year aggregate + behaviour
│   │   │   │   ├── Herd.h            ← M1 — cohorts + M4a muster state machine
│   │   │   │   ├── Bore.h            ← M2 — water source + failure/repair
│   │   │   │   ├── Trough.h          ← M2 — water endpoint (placeholder)
│   │   │   │   ├── Tank.h            ← M2 — capacity (placeholder)
│   │   │   │   ├── Player.h          ← M3 — role + cash + skills
│   │   │   │   ├── PriceTable.h      ← M3 — cattle price curve
│   │   │   │   ├── Supplement.h      ← M3 — lick/molasses/feed
│   │   │   │   ├── Hand.h            ← M4a — AI station hand
│   │   │   │   ├── Vehicle.h         ← M4a — 6 movement types (foot..chopper)
│   │   │   │   ├── Fence.h           ← M4a — paddock boundary + integrity
│   │   │   │   ├── Road.h            ← M4a — paddock link + grade quality
│   │   │   │   ├── StationEvent.h    ← M5 — drought/flood/fire/storm/breakdown
│   │   │   │   ├── Species.h         ← M6 — 13 birds + 3 mammals registry
│   │   │   │   ├── PestPresence.h    ← M6 — dingo/cat/pig per paddock
│   │   │   │   ├── InvasivePresence.h ← M6 — parkinsonia/vine/acacia coverage
│   │   │   │   ├── WorkMachine.h     ← M8 — 7 machinery types
│   │   │   │   └── Sensor.h          ← M8 — rain/tank/bore sensors
│   │   │   └── Systems/
│   │   │       ├── ConditionSystem.h   ← M1 — herd condition drift
│   │   │       ├── WaterSystem.h       ← M2 — bore decay + paddock waterAccess
│   │   │       ├── EconomySystem.h     ← M3 — wages + sale + year-end summary
│   │   │       ├── MusteringSystem.h   ← M4a — muster state machine (stub)
│   │   │       ├── EventSystem.h       ← M5 — daily event roll + drought
│   │   │       ├── BreedingSystem.h    ← M5 — joining → gestation → calving
│   │   │       ├── WildlifeSystem.h    ← M6 — pests + invasives + bird logging
│   │   │       ├── ProgressionSystem.h ← M7 — year-end eval + role transition
│   │   │       └── SensorSystem.h      ← M8 — battery tick + readings
│   │   └── Private/                  ← .cpp implementations
│   └── TaraGame/                     ← engine-aware presentation layer
│       ├── TaraGame.Build.cs         ← depends on Core, CoreUObject, Engine, … + TaraSimCore
│       ├── Public/
│       │   ├── Bridge/
│       │   │   └── TaraSimSubsystem.h  ← GameInstanceSubsystem; owns FStation;
│       │   │                              22 BlueprintCallable + ~30 BlueprintPure +
│       │   │                              40+ multicast delegates
│       │   ├── Actors/
│       │   │   ├── PaddockActor.h    ← renders one FPaddock; updates GrassFreshness
│       │   │   ├── BoreActor.h       ← windmill + tank + trough silhouette
│       │   │   └── CattleActor.h     ← placeholder bovine
│       │   ├── Pawns/
│       │   │   └── QuadBikePawn.h    ← Phase 1 player vehicle (arcade feel)
│       │   └── UI/
│       │       └── TaraHUDWidget.h   ← time + herd + paddock readout
│       └── Private/                  ← .cpp implementations
├── Config/                           ← DefaultEngine.ini, DefaultGame.ini
├── Content/                          ← .uassets (created/edited in the Editor)
├── .github/workflows/
│   └── sim-isolation-check.yml       ← CI grep check — no engine headers in TaraSimCore
└── PHASE1_EDITOR_TODO.md             ← Editor-side work to finish Phase 1
```

## The architectural invariant

The sim core (`Source/TaraSimCore/`) declares **only `Core` as a build dependency**. It has no `UCLASS`, no `UObject`, no Engine/GameFramework/UMG/Slate includes. This is the same invariant the 2D project held across all 8 milestones (`grep -r 'import.*phaser' src/sim/` returned nothing), reimplemented at UE5's module-system level.

**The CI workflow at `.github/workflows/sim-isolation-check.yml` enforces this** — every push + PR runs:

```bash
grep -rE '#include\s*"(Engine|CoreUObject|GameFramework|UMG|Slate)/' Source/TaraSimCore/
```

Trying to bring engine headers into the sim layer fails the build. **The invariant held byte-for-byte through the entire 8/8 port** — not one TaraSimCore source file imports anything beyond Core.

If you ever want to "just quickly add a `UPROPERTY()` to FPaddock so I can edit it in the Editor": stop. The pattern is to put the engine-aware Actor (`APaddockActor`) in `TaraGame` and have it hold a non-owning pointer to the pure-sim `FPaddock`.

## Driving the sim from the engine

The bridge subsystem owns one `FStation` and exposes its full surface to Blueprints / Widgets / Actors. Typical wire-up patterns:

**Day-tick (engine ticks real-time → sim advances):**
```cpp
// BP_TaraGameMode::Tick → call once per frame:
Subsystem->AdvanceRealTime(DeltaSeconds);
// The bridge converts real seconds → in-game minutes via FSeasonClock's tunable
// RealSecondsPerInGameDay. When the sleep boundary (22:00) trips, the bridge
// runs Station::TickDay() which fires every system in the right order.
```

**Reading sim state in UMG:**
```cpp
// In WBP_TaraHUD::Tick or via BindEvent:
auto* Sub = UGameInstance::GetSubsystem<UTaraSimSubsystem>(...);
HerdSizeText->SetText(FText::AsNumber(Sub->GetHerdSize()));
GrassText->SetText(FText::AsNumber(Sub->GetPaddockGrassKgPerHa(TEXT("Holding"))));
DroughtText->SetText(FText::AsNumber(Sub->GetCurrentDroughtSeverity()));
```

**Reacting to sim events:**
```cpp
// In an actor's BeginPlay:
Sub->OnDayEnded.AddUObject(this, &AMyActor::HandleDayEnded);
Sub->OnBoreFailed.AddUObject(this, &AMyActor::HandleBoreFailed);
Sub->OnEventStarted.AddUObject(this, &AMyActor::HandleEvent); // drought/flood/fire/...
Sub->OnRoleChanged.AddUObject(this, &AMyActor::HandleRoleChange); // ringer→manager→owner
```

**Player-driven actions (UMG button → sim):**
```cpp
Sub->BuySupplement(0);                       // 0=Lick, 1=Molasses, 2=Feed
Sub->SellCattle(20);                          // returns $ earned
Sub->StartMuster("Holding", 3, {"hand-1"}, 2, 30, 0);  // 3D layer computes args
Sub->ResolveBadWeatherDecision("feed");       // drought modal choice
Sub->LogBird("emu");                          // researcher payout
Sub->BuyProperty();                           // manager → owner ($250k)
Sub->BuyWorkMachine(4);                       // 4=Grader ($95k)
```

## Sim port progress

Per [`sim-port-audit.md`](https://github.com/GibbHubb/Tara_Station/blob/main/docs/3d-pivot/sim-port-audit.md): every entity + system from the 2D M1-M8 stack is now ported.

| M | Status | Commit | What it added |
|---|---|---|---|
| **M1** condition / paddock / herd | ✅ Phase 0 | (initial scaffold) | Paddock + CattleCohort + Herd + ConditionSystem + EventBus + SeasonClock |
| **M2** water | ✅ | `dd413c0` | Bore + Trough + Tank + WaterSystem (decay + failure + access derivation) |
| **M3** economy | ✅ | `2af7d8c` | Player + PriceTable + Supplement + EconomySystem (wage drip + sale + year-end summary) |
| **M4a** mustering | ✅ | `25797f5` | Hand + Vehicle (6 types) + Fence + Road + Herd muster state + MusteringSystem stub |
| **M5** events + breeding | ✅ | `0ba1f1f` | StationEvent + EventSystem (drought/flood/fire/storm/breakdown + calendar) + BreedingSystem (joining → 280d gestation → calving) |
| **M6** wildlife | ✅ | `db5c5c0` | Species (13 birds + 3 mammals) + PestPresence + InvasivePresence + WildlifeSystem |
| **M7** progression | ✅ | `5be6b5c` | ProgressionSystem (year-end 0-100 score + role transition + buy-property + bankruptcy) |
| **M8** machinery + sensors | ✅ | `9d877d6` | WorkMachine (7 types) + Sensor (3 kinds) + SensorSystem (battery + readings) |
| **pacing dials** | ✅ | `bc4fc99` | FSeasonClock::AdvanceRealTime + tunable RealSecondsPerInGameDay (30min default) + ProgressionSystem hypothesis constant |

🟡 calls held under port (state shapes carried forward; spatial reinterpretation is the Editor-side Phase 2 work). 🔴 calls held without scope creep — `MusteringSystem.computeMuster` deliberately not implemented (the stub keeps the state machine; outcome math lives in the 3D layer); Avatar entity deleted entirely (player Pawn is engine-side).

## Phase plan

| Phase | Adds | Sim status |
|---|---|---|
| **0** Pre-production | UE5 project + two-module scaffold + sim skeleton + bridge + CI | ✅ done |
| **1** Vertical slice | One paddock + gold grass + drivable quad + working bore + cottage → shed → quad → paddock → bore → return daily loop | C++ done; Editor work in [`PHASE1_EDITOR_TODO.md`](PHASE1_EDITOR_TODO.md) |
| **2** Working station | Curated 13-paddock layout (real Tara names) + shed + workers' cottage hub + permanent river along real central watercourse + wet/dry shed-action-menu flip | Sim ready (M2 + M3 + M4a all in) |
| **3** Visual authenticity | Cloudless dusk gradient locked + vegetation scatter + wildlife sprinkle | — (presentation only) |
| **4** Landmarks + gorge | Monolith East + spire field Limestone NE + gorge NW near Granada — all reachable on foot/quad with no loading screen | Blocked by `TS-3D-PHASE4-cultural-review` (monolith consultant) |
| **5+** Depth + publish | Branding crush + weaning + weaner school + preg-test sub-scenes + drought visibly bleaches + flood fills creeks + bird researcher + 2-way radio + chopper-tier muster + M8 capital + multi-property | Sim ready (M5 + M6 + M7 + M8 all in) |

## Save/load

Single rolling autosave on every `DayEnded` event, mirroring the 2D project's pattern. JSON via `FFileHelper`:

```
Tara_Station_3D/Saved/SaveGames/tara-save-3d-v8-m8.json
```

Schema versioned. When the schema changes (`TARA_SIM_SAVE_SCHEMA_VERSION` in `Source/TaraSimCore/Public/Station.h`), mismatched older files discard cleanly via `FStation::FromJson` returning `nullptr` — a fresh game starts without crashing. Mirrors the 2D versioned-localStorage-key pattern.

## Repo cross-reference

- **2D design reference**: [`GibbHubb/Tara_Station`](https://github.com/GibbHubb/Tara_Station)
- **Pivot brief** (single source of truth): [`Tara_Station/3D_PROJECT_BRIEF.md`](https://github.com/GibbHubb/Tara_Station/blob/main/3D_PROJECT_BRIEF.md)
- **Companion briefs**: [`MAP_BRIEF.md`](https://github.com/GibbHubb/Tara_Station/blob/main/docs/3d-pivot/MAP_BRIEF.md) (real-Tara spatial grounding) · [`CORE_LOOP.md`](https://github.com/GibbHubb/Tara_Station/blob/main/docs/3d-pivot/CORE_LOOP.md) (daily/yearly rhythm) · [`landmarks.md`](https://github.com/GibbHubb/Tara_Station/blob/main/docs/3d-pivot/landmarks.md) · [`watershed.md`](https://github.com/GibbHubb/Tara_Station/blob/main/docs/3d-pivot/watershed.md)
- **Backlog tracker**: [`GibbHubb/backlog-bandit`](https://github.com/GibbHubb/backlog-bandit) — search `TS-3D-` rows
