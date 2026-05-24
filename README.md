# Tara_Station_3D — Outback Life (Unreal Engine 5)

A 3D cattle-station life-sim built on the simulation already developed across M1–M8 of the 2D Phaser/TypeScript prototype ([`GibbHubb/Tara_Station`](https://github.com/GibbHubb/Tara_Station)). The 2D build is the **design reference**; this is the publishable 3D rebuild.

> **Status: Phase 0 + Phase 1 C++ scaffold landed (2026-05-24).** The engine-agnostic sim core ports M1 (cattle condition + grass-floor) cleanly; the bridge subsystem owns one `FStation` and exposes it to the engine layer; Phase 1 Actors / Pawn / HUD widget compile and can be placed in a level. **Editor-side work to bring up the vertical slice** is documented in [`PHASE1_EDITOR_TODO.md`](PHASE1_EDITOR_TODO.md).

See the 2D repo's [`3D_PROJECT_BRIEF.md`](https://github.com/GibbHubb/Tara_Station/blob/main/3D_PROJECT_BRIEF.md) for the single source of truth on direction, and [`docs/3d-pivot/`](https://github.com/GibbHubb/Tara_Station/tree/main/docs/3d-pivot) for the sim-port audit, backlog reconciliation, 3D milestone plan, and UE5 architecture rationale.

---

## Prerequisites

- **Unreal Engine 5.3** (or 5.4 — update `EngineAssociation` in `Tara_Station_3D.uproject` to match what you have installed).
- **Visual Studio 2022** with the **Game development with C++** workload (`MSVC v143`, Windows 10/11 SDK, C++ AddressSanitizer optional).
- Git.

(Rider is fine instead of Visual Studio if you prefer. macOS Xcode toolchain also works.)

## First-time setup

```bash
# 1. From this directory:
right-click Tara_Station_3D.uproject → Generate Visual Studio project files
# (or from terminal: UnrealBuildTool / Engine helper script — varies by host OS)

# 2. Open the .sln in Visual Studio.

# 3. Build "Tara_Station_3DEditor — Development Editor" once.
#    First build pulls down + compiles Engine headers; subsequent builds are fast.

# 4. Double-click Tara_Station_3D.uproject — opens the Editor.

# 5. Follow PHASE1_EDITOR_TODO.md to set up the level, materials, and assets.
```

## Project layout

```
Tara_Station_3D/
├── Tara_Station_3D.uproject          ← project descriptor (UE5.3+)
├── Source/
│   ├── Tara_Station_3D.Target.cs     ← runtime target
│   ├── Tara_Station_3DEditor.Target.cs ← editor target
│   ├── TaraSimCore/                  ← engine-agnostic simulation (NO ENGINE)
│   │   ├── TaraSimCore.Build.cs      ← declares ONLY "Core" — enforced
│   │   ├── Public/
│   │   │   ├── EventBus.h            ← typed pub/sub (TFunction-based)
│   │   │   ├── EventPayloads.h       ← struct-per-event-type
│   │   │   ├── SeasonClock.h         ← year/day/hour/minute + season
│   │   │   ├── Station.h             ← orchestrator (owns entities + systems)
│   │   │   ├── Entities/
│   │   │   │   ├── Paddock.h
│   │   │   │   ├── CattleCohort.h
│   │   │   │   └── Herd.h
│   │   │   └── Systems/
│   │   │       └── ConditionSystem.h
│   │   └── Private/                  ← .cpp implementations
│   └── TaraGame/                     ← engine-aware presentation layer
│       ├── TaraGame.Build.cs         ← depends on Core, CoreUObject, Engine, … + TaraSimCore
│       ├── Public/
│       │   ├── Bridge/
│       │   │   └── TaraSimSubsystem.h  ← GameInstanceSubsystem owns the FStation
│       │   ├── Actors/
│       │   │   ├── PaddockActor.h    ← renders one FPaddock; updates GrassFreshness on tick
│       │   │   ├── BoreActor.h       ← windmill + tank + trough silhouette
│       │   │   └── CattleActor.h     ← placeholder bovine
│       │   ├── Pawns/
│       │   │   └── QuadBikePawn.h    ← Phase 1 player vehicle (arcade feel)
│       │   └── UI/
│       │       └── TaraHUDWidget.h   ← YYYY/DDD time + herd + paddock readout
│       └── Private/                  ← .cpp implementations
├── Config/                           ← DefaultEngine.ini, DefaultGame.ini
├── Content/                          ← .uassets (created/edited in the Editor)
├── .github/workflows/
│   └── sim-isolation-check.yml       ← CI grep check — no engine headers in TaraSimCore
└── PHASE1_EDITOR_TODO.md             ← Editor-side work to finish Phase 1
```

## The architectural invariant

The sim core (`Source/TaraSimCore/`) declares **only `Core` as a build dependency**. It has no `UCLASS`, no `UObject`, no Engine/GameFramework/UMG/Slate includes. This is the same invariant the 2D project held across all 8 milestones (`grep -r 'import.*phaser' src/sim/` returned nothing), reimplemented at UE5's module-system level.

**The CI workflow at `.github/workflows/sim-isolation-check.yml` enforces this.** It greps for forbidden includes in `Source/TaraSimCore/` on every push + PR. Trying to bring engine headers into the sim layer fails the build.

If you ever want to "just quickly add a `UPROPERTY()` to FPaddock so I can edit it in the Editor": stop. The pattern is to put the engine-aware Actor (`APaddockActor`) in `TaraGame` and have it hold a non-owning pointer to the pure-sim `FPaddock`.

## What ports from 2D + what's new

Per [`sim-port-audit.md`](https://github.com/GibbHubb/Tara_Station/blob/main/docs/3d-pivot/sim-port-audit.md): ~70% of the 2D sim's entities and systems port unchanged in logic. The 🟡 work concentrates in spatial topology (Paddock as a polygon, not a slot; Vehicle as drivable, not a multiplier); the only 🔴 rebuilds are `MusteringSystem.computeMuster` (the abstract outcome calc → real-time spatial gameplay) and the Avatar entity (→ UE5 Character).

Phase 0 has shipped M1 (cattle condition + grass-floor). Phase 1 adds the spatial layer for one paddock. Subsequent phases per [`3d-milestone-plan.md`](https://github.com/GibbHubb/Tara_Station/blob/main/docs/3d-pivot/3d-milestone-plan.md):

| Phase | Adds |
|---|---|
| **0** | UE5 project + two-module scaffold + sim skeleton port + bridge + CI ← **DONE** |
| **1** | Vertical slice — one paddock + gold grass + drivable quad + working bore + day-tick + dusk gradient ← **C++ DONE; Editor work in PHASE1_EDITOR_TODO.md** |
| 2 | Multi-paddock station, homestead + props, mustering as spatial activity, M2+M3+M4a port |
| 3 | Visual authenticity pass — cloudless dusk gradient locked, vegetation scatter, wildlife sprinkle |
| 4 | The gorge — continuous transition from station to rocky country |
| 5+ | M5 events + M6 wildlife + M7 progression + M8 capital deep-rendered into the 3D world |

## Save/load

Single rolling autosave on every `DayEnded` event, mirroring the 2D project's pattern. JSON via `FFileHelper`:

```
Tara_Station_3D/Saved/SaveGames/tara-save-3d-v1-phase0.json
```

Schema is versioned (`tara-save-3d-v1-phase0` for Phase 0). When the schema changes (Phase 1+), the old save discards cleanly and a fresh game starts. Mirrors the 2D pattern.

## Git

This local working directory is **not yet a git repo**. When ready:

```bash
git init
git add -A
git commit -m "feat: initial Phase 0 + Phase 1 C++ scaffold"
gh repo create GibbHubb/Tara_Station_3D --public --source=. --push
```

The `.gitignore` excludes UE5's `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/` — only source + content + config tracked.

## Repo cross-reference

- 2D design reference: [`GibbHubb/Tara_Station`](https://github.com/GibbHubb/Tara_Station)
- Pivot brief: [`Tara_Station/3D_PROJECT_BRIEF.md`](https://github.com/GibbHubb/Tara_Station/blob/main/3D_PROJECT_BRIEF.md)
- Backlog tracker: [`GibbHubb/backlog-bandit`](https://github.com/GibbHubb/backlog-bandit) (`TS-3D-PHASE0` / `TS-3D-PHASE1` rows)
