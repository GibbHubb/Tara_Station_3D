using UnrealBuildTool;

// TaraSimCore — the engine-agnostic simulation module.
//
// THE ARCHITECTURAL INVARIANT: This module depends ONLY on Core.
// No CoreUObject, no Engine, no GameFramework, no UMG, no Slate.
// The build system itself enforces the "sim layer has no engine imports"
// rule from the 2D project (where `grep -r 'import.*phaser' src/sim/`
// returned nothing across all 8 milestones).
//
// If you find yourself wanting to add CoreUObject here so you can use
// UObject or UFUNCTION: stop. That kind of code belongs in TaraGame.
// The bridge (UTaraSimSubsystem in TaraGame) wraps this pure sim.

public class TaraSimCore : ModuleRules
{
	public TaraSimCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			// DELIBERATELY OMITTED: CoreUObject, Engine, GameFramework, UMG,
			// Slate, SlateCore, InputCore. Adding any of these forfeits the
			// engine-agnostic invariant — see PROJECT_BRIEF.md §2.
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			// Same rule. Core only.
		});
	}
}
