# Session summary — 2026-05-24 + 25 (sim port + Editor prep)

> **For:** future-Max or any agent picking this repo up cold. Single-doc handoff explaining what shipped over the two-day session 2026-05-24 → 2026-05-25, what's ready to execute, and what's outstanding.
> **Last updated:** 2026-05-25
> **HEAD:** Tara_Station_3D `0d69faf` · backlog_bandit `890e19b`

---

## TL;DR

- **Sim port COMPLETE 8/8** — M1 condition + M2 water + M3 economy + M4a mustering + M5 events/breeding + M6 wildlife + M7 progression + M8 machinery/sensors all live in `Source/TaraSimCore/` (pure C++, zero engine imports — invariant enforced by CI grep).
- **Phase 5+ prerequisites pre-shipped** — `bLactating`, `BrandedDay`, `bHorned` cohort fields + `WeanCohort` + `SplitCohort` Station actions + `EffectiveMusterDifficulty` helper + `LactationExtraDecay` in ConditionSystem.
- **Pacing dials shipped + tunable** — `FSeasonClock::DefaultRealSecondsPerInGameDay = 1800` (30 min/day default) + `Subsystem->AdvanceRealTime(Delta)` is the canonical day-tick wiring. `SetSecondsPerInGameDay(seconds)` is the settings-UI hook (clamped 60..7200).
- **Bridge surface comprehensive** — ~30 BlueprintCallable actions, ~40 BlueprintPure getters, ~50 multicast delegates. Documented in [BRIDGE_API.md](BRIDGE_API.md).
- **Sim smoke test** — run `Tara.RunSmokeTest 365` in the in-game console; PASS/FAIL via UE_LOG with full invariant check (clock advance, cohort counts, save round-trip, SplitCohort head-count preservation, WeanCohort state flip, bHorned distribution).
- **Pre-built engine-side scaffolds** — `AInteractableActor` base + 3 concrete subclasses (`ABoreCheckInteract` / `AFenceRepairInteract` / `ARoadGradeInteract`) + `AVehicleSlotActor` + `AWeanerCornerVolume` + `URadioBanner` UMG + updated `UTaraHUDWidget` with full 8/8 sim readout.
- **8 plans drafted + agreed `[P]`** for the next chunks of work (see "Plans queue" below).
- **3 click-by-click Editor TODO docs** for Phase 1 + Phase 2 paddocks + Phase 2 watershed + Phase 2 shed-action menu.

The sim layer + bridge are done. Everything outstanding is Editor work (`.uasset` creation, UMG layout, Blueprint wiring, level/Landscape painting, asset import, PIE testing).

---

## How to pick up cold

### If you're Max coming back to this in UE5

1. Open `Tara_Station_3D.uproject` in the Editor (UE5.7+).
2. In the Output Log, look for `[TaraSim] Initialized. Year 1 Day 1 06:00 — herd 8 head` — confirms the bridge is loading + sim is alive.
3. Open the in-game console (backtick during PIE) and type `Tara.RunSmokeTest 365`. PASS confirms 8/8 sim parity + invariants hold over a simulated year.
4. From here, your next decision is which plan to execute. **Recommend:** finish `TS-3D-TWO-SEASON-MODE` (C++ already shipped; only Editor steps remain — see [PHASE2_SHED_EDITOR_TODO.md](PHASE2_SHED_EDITOR_TODO.md)). It's the smallest scope + uses the existing C++ scaffolds.

### If you're an agent picking up cold

1. Read [CLAUDE.md](../../backlog_bandit/CLAUDE.md) — operating manual for backlog_bandit (this repo's project tracker).
2. Read this doc + [BRIDGE_API.md](BRIDGE_API.md) for the bridge surface.
3. Run `git log --oneline -20` here + in backlog_bandit to see what landed in what order.
4. The Ready Queue (status `[A]`) in backlog_bandit BACKLOG.md is the next-up agreed work; everything else `[P]` needs Max's agreement before executing.

---

## Sim port commit chain (2026-05-24 + 25)

| Phase | Commit | What it added |
|---|---|---|
| M2 water | (pre-session) `dd413c0` | WaterSystem + Bore + Trough + Tank entities |
| M3 economy | (pre-session) `2af7d8c` | EconomySystem + Player + PriceTable + Supplement |
| M4a mustering | `25797f5` | Hand + Vehicle + Fence + Road + MusteringSystem stub + Herd muster state |
| M5 events+breeding | `0ba1f1f` | StationEvent + EventSystem + BreedingSystem |
| M6 wildlife | `db5c5c0` | Species registry + PestPresence + InvasivePresence + WildlifeSystem |
| M7 progression | `5be6b5c` | ProgressionSystem + year-end eval + role transitions + bankruptcy |
| M8 machinery+sensors | `9d877d6` | WorkMachine + Sensor + SensorSystem |
| Pacing dials | `bc4fc99` | RealSecondsPerInGameDay + AdvanceRealTime + YearsToOwnerHypothesis |
| Phase 5+ prereqs | `0f59cc9` | bLactating + BrandedDay + bHorned + WeanCohort + SplitCohort + lactation drift |

Then Editor prep:

| Block | Commit | What |
|---|---|---|
| Map Brief integration | `bd331bb` | MAP_BRIEF.md saved + integrated into landmarks/watershed/milestone-plan/world docs; UE5 5.4+ locked; cultural-consultant review flagged |
| Core Loop Brief integration | `b66d101` | CORE_LOOP.md saved + 2-mode year locked + cattle calendar locked |
| CORE_LOOP §7 answers | `1c21e35` | Max's confirmed answers: bad-wet occasional/not constant; mustering means quad+motorcycle+horse+chopper+2-way-radios; day length 25-45min TUNABLE; ~4yr arc TUNABLE |
| TS-3D-TWO-SEASON-MODE C++ | `19c385f` + `954b2ab` | UShedActionMenu + FShedActionRow + OnSeasonChanged synth delegate + bankruptcy override + default rows fallback + CSV + PHASE2_SHED_EDITOR_TODO |
| Bridge API doc | `e752087` | BRIDGE_API.md single-page reference + MusterTrainedness + BreederStage + EffectiveMusterDifficulty |
| Sim smoke test + Phase 1 refresh | `4f96127` | FSimSmokeTest + Tara.RunSmokeTest + PHASE1_EDITOR_TODO refresh |
| 4 actor + UMG bases | `1459f0d` | AInteractableActor + AVehicleSlotActor + URadioBanner + AWeanerCornerVolume |
| Editor TODOs + CSV + getters | `2b2a76d` | PHASE2_PADDOCKS_EDITOR_TODO + PHASE2_WATERSHED_EDITOR_TODO + DT_BossOrders.csv (20 lines) + 11 enumeration getters |
| Ringer-work subclasses | `0d69faf` | ABoreCheckInteract + AFenceRepairInteract + ARoadGradeInteract |
| HUD widget refresh | (next commit) | TaraHUDWidget extended to surface full 8/8 sim state |

---

## What's in `Source/TaraSimCore/`

Pure C++, zero engine imports. Invariant: `grep -rE '#include\s*"(Engine|CoreUObject|GameFramework|UMG|Slate)/' Source/TaraSimCore/` returns zero matches. CI workflow (`.github/workflows/sim-isolation-check.yml`) enforces this on every push.

```
Public/
├── EventBus.h           templated typed pub/sub (TFunction-based)
├── EventPayloads.h      ~50 plain-struct payloads
├── SeasonClock.h        year/day/hour/minute + RealSecondsPerInGameDay + AdvanceRealTime
├── Station.h            orchestrator — owns all entities + systems
├── Entities/
│   ├── Paddock.h           M1 area + grass + intake/growth
│   ├── CattleCohort.h      M1 cohort aggregate + Phase 5+ fields
│   ├── Herd.h              M1 cohorts + M4a muster state machine
│   ├── Bore.h              M2 water source + failure/repair
│   ├── Trough.h            M2 placeholder
│   ├── Tank.h              M2 placeholder
│   ├── Player.h            M3 role + cash + skills
│   ├── PriceTable.h        M3 cattle price curve
│   ├── Supplement.h        M3 lick/molasses/feed
│   ├── Hand.h              M4a AI hand (skill + wage + hired)
│   ├── Vehicle.h           M4a 6 types (foot..chopper)
│   ├── Fence.h             M4a paddock boundary + integrity
│   ├── Road.h              M4a paddock link + grade quality
│   ├── StationEvent.h      M5 drought/flood/fire/storm/breakdown/rodeo/draft
│   ├── Species.h           M6 13 birds + 3 mammals registry
│   ├── PestPresence.h      M6 dingo/cat/pig per paddock
│   ├── InvasivePresence.h  M6 parkinsonia/vine/acacia coverage
│   ├── WorkMachine.h       M8 7 types (tractor..molasses-truck)
│   └── Sensor.h            M8 rain/tank/bore sensors
└── Systems/
    ├── ConditionSystem.h    M1 herd condition drift + Phase 5+ lactation drift
    ├── WaterSystem.h        M2 bore decay + paddock waterAccess
    ├── EconomySystem.h      M3 wages + sale + year-end summary
    ├── MusteringSystem.h    M4a state machine stub (no computeMuster — 3D decides spatially)
    ├── EventSystem.h        M5 daily event roll + drought severity + grass-growth scale
    ├── BreedingSystem.h     M5 joining window + gestation + calving
    ├── WildlifeSystem.h     M6 pest growth + invasive spread + bird logging
    ├── ProgressionSystem.h  M7 year-end eval + role transition + bankruptcy
    └── SensorSystem.h       M8 battery tick + per-kind readings

Save schema: tara-save-3d-v9-phase5-prereqs (mismatched older files discard cleanly)
```

---

## What's in `Source/TaraGame/`

Engine-aware bridge + UI + Actor scaffolds. Depends on Core, CoreUObject, Engine, InputCore, EnhancedInput, UMG, Slate, SlateCore, TaraSimCore.

**Note:** the Water plugin is NOT enabled by default — TS-3D-PHASE2-watercourse execution will enable it + add `"Water"` to Build.cs.

```
Public/
├── Bridge/
│   └── TaraSimSubsystem.h   UGameInstanceSubsystem — owns FStation,
│                              re-emits FEventBus events as UE multicast
│                              delegates, exposes ~30 BlueprintCallable +
│                              ~40 BlueprintPure surfaces
├── Actors/
│   ├── PaddockActor.h          Phase 1 — paddock visualisation
│   ├── BoreActor.h              Phase 1 — bore visualisation
│   ├── CattleActor.h            Phase 1 — placeholder cattle
│   ├── InteractableActor.h      Round E — base for ringer-work interactables
│   ├── VehicleSlotActor.h       Round E — shed-cottage vehicle bay
│   ├── WeanerCornerVolume.h     Round E — TS-3D-WEANER-SCHOOL paddock-corner
│   ├── BoreCheckInteract.h      Round K — sim-integrated subclass
│   ├── FenceRepairInteract.h    Round K — sim-integrated subclass
│   └── RoadGradeInteract.h      Round K — sim-integrated subclass
├── Pawns/
│   └── QuadBikePawn.h           Phase 1 — drivable quad
├── UI/
│   ├── TaraHUDWidget.h          MAIN HUD — full 8/8 sim readout (refreshed)
│   ├── ShedActionMenu.h         TS-3D-TWO-SEASON-MODE base + FShedActionRow + EShedActionSeason
│   └── RadioBanner.h            Round E — TS-3D-RADIO top-of-screen overlay
└── Diagnostics/
    └── SimSmokeTest.h           Round C — Tara.RunSmokeTest backing class
```

---

## Plans queue (in `backlog_bandit/plans/`)

**Status as of 2026-05-25:**

| ID | Status | Notes |
|---|---|---|
| `TS-3D-TWO-SEASON-MODE` | `[~]` in flight | C++ DONE; Editor steps await Max — see PHASE2_SHED_EDITOR_TODO |
| `TS-3D-PHASE2-shed-cottage` | `[P]` planned | Foundation for TWO-SEASON-MODE in level form (cottage + shed actors) |
| `TS-3D-PHASE2-paddocks` | `[P]` planned | 13 real-named paddocks + sim seed rewrite; click-by-click in PHASE2_PADDOCKS_EDITOR_TODO |
| `TS-3D-PHASE2-watercourse` | `[P]` planned | Water plugin + river + creek + dams; click-by-click in PHASE2_WATERSHED_EDITOR_TODO |
| `TS-3D-RINGER-WORK` | `[P]` planned | 7 world-space interactables — 3 C++ subclasses shipped (Round K), 4 are pure BP |
| `TS-3D-RADIO` | `[P]` planned | 2-way radio comms; URadioBanner shipped; CSV pool shipped |
| `TS-3D-BRANDING` | `[P]` planned | Phase 5+ yard sub-scene; bHorned + BrandedDay fields shipped |
| `TS-3D-WEANER-SCHOOL` | `[P]` planned | Phase 5+ wean + schooling; bLactating + MusterTrainedness shipped + AWeanerCornerVolume shipped |
| `TS-3D-PREG-TEST` | `[P]` planned | Phase 5+ preg-test draft; SplitCohort + BreederStage shipped |
| `TS-3D-PHASE4-cultural-review` | `[!]` blocked | Awaiting consultant identification — Phase 4 lead time |
| `TS-3D-PACING-TUNE` | done | Shipped at `bc4fc99` |
| `TS-3D-CORE-LOOP-CONFIRM` | done | All 4 [CONFIRM] markers answered by Max in CORE_LOOP §7 |

`agree TS-3D-<ID>` flips a `[P]` row to `[A]` and moves it to the Ready Queue; `execute <ID>` flips `[A]` → `[~]`.

---

## Open questions (still pending Max's answer)

These haven't blocked any work but should be locked before the relevant phase ships:

- **Fictional names** for the monolith / gorge / spire field landmarks (must not echo Uluru / Katherine / Pinnacles names). Lands in [world.md](https://github.com/GibbHubb/Tara_Station/blob/main/docs/product/world.md).
- **Drought-severity → river-volume curve shape** (linear vs stepped — defer to Phase 5+ wet/dry wiring).
- **Cultural-sensitivity consultant** for the monolith design (`TS-3D-PHASE4-cultural-review`). 4-8 weeks lead time.
- **2D playtest timing** — when Max sits down with the 2D `GibbHubb/Tara_Station` build for tuning carry-over to 3D.
- **Per-plan §8 open questions** — each agreed plan documents specific open Qs in its risks section (paddock area numbers, cottage-shed distance, fence cost values, etc.). Most have documented defaults that apply at execute-time unless Max overrides.

---

## Locked decisions (2026-05-24/25)

Already in memory + sprinkled through the planning docs. Single-source-of-truth list:

- **UE5 5.4+** (newest stable — Max installed 5.7.4 which exceeds the lock; fine)
- **Tara_Station_3D repo private** before first asset commits
- **Real Tara Pastoral Lease topographic map** (1:17,500 GDA94/MGA54) is the spatial source of truth — real paddock names, real watering points, fictional landmarks placed on real terrain (monolith East, spires Limestone NE, gorge NW range near Granada, river along real central watercourse)
- **TS14-shedscene PROMOTED to foundational** (was "absorbed into Phase 2") — Shed + workers' cottage is the daily-anchor hub
- **Cultural consultant review** flagged for Phase 4 lead time (monolith design)
- **Wet-season reliability**: generally consistent with occasional bad years — bad-wet probability in EventSystem ~15-25%/yr starting hypothesis (CORE_LOOP §7)
- **Mustering means** (in order of everyday use): quad + motorcycle / horses / occasional expensive helicopter / **2-way radios** for comms
- **Day length** = TUNABLE; starting hypothesis 25-45 real-minutes per in-game day (default 30 min/day in code)
- **Ownership arc** = TUNABLE; ~4 in-game years (≈1 ringer + ~2 manager + then owner)
- **Watercourse plan** uses UE5 Water plugin (not hand-rolled spline-mesh) per revision 2026-05-25
- **Grey-out (not hide) off-season shed actions** so the player learns the rhythm by SEEING what's not available right now

---

## Critical paths if you're picking up cold

Most likely use-cases for a future session:

1. **"I just got UE5 working — what do I do first?"** → Open the project; verify `[TaraSim] Initialized` log line; run `Tara.RunSmokeTest 365`; then walk [PHASE1_EDITOR_TODO.md](PHASE1_EDITOR_TODO.md) for the screenshot gate.
2. **"I want to execute one of the agreed Phase 2 plans"** → Pick TS-3D-PHASE2-shed-cottage (foundation) or TS-3D-TWO-SEASON-MODE (smallest scope, mostly done). Each has a corresponding `PHASE2_*_EDITOR_TODO.md` at the repo root.
3. **"I want to wire up a UMG widget"** → Read [BRIDGE_API.md](BRIDGE_API.md); use `BindWidgetOptional` for safety; refresh on `OnDayEnded` for most stats; refresh on the specific delegate (`OnMoneyChanged`, `OnMusterCompleted`, etc.) for reactive events.
4. **"I want to extend the sim"** → Add field to entity → add JSON round-trip → bump save schema → add BlueprintPure getter to `UTaraSimSubsystem` → add to `BRIDGE_API.md` → run smoke test → commit.

---

## What's deliberately NOT in scope yet

- Multi-property gameplay (M7 sim supports it; Editor work is far future).
- AI hand behavioural responses to issued radio orders (TS-3D-RADIO is texture-only for now).
- Real photogrammetric Tara aerial → Landscape import (we hand-paint to match the lease map).
- Genetic / pedigree tracking on individual animals (cohorts only).
- Networking / multiplayer (single-player only).
- VR / mobile platforms (PC standalone only).

---

## Repo cross-references

- **Tara_Station_3D** (this repo): the UE5 build. Private. `https://github.com/GibbHubb/Tara_Station_3D`
- **Tara_Station** (2D design reference): `https://github.com/GibbHubb/Tara_Station` — the M1-M8 Phaser/TypeScript build that became this sim port. Stays maintained.
- **backlog-bandit** (project tracker): `https://github.com/GibbHubb/backlog-bandit` — BACKLOG.md, plans/, done_log/, PROJECTS.md.

When in doubt about any design choice, follow this hierarchy:
1. The lock in **CORE_LOOP.md §7** (Max's confirmed answers)
2. The lock in **MAP_BRIEF.md** (real-Tara spatial source of truth)
3. The 2D project's behaviour at `GibbHubb/Tara_Station` (design reference)
4. UE5 community best-practice
