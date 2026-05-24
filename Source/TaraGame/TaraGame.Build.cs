using UnrealBuildTool;

// TaraGame — the engine-aware presentation layer.
//
// Depends on the full UE5 runtime AND on TaraSimCore. The one-way dependency
// (TaraSimCore knows nothing about TaraGame) is what keeps the sim portable.
// New rule: any code that drags Engine concepts into a sim entity belongs HERE,
// not in TaraSimCore.

public class TaraGame : ModuleRules
{
	public TaraGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"UMG",
			"Slate",
			"SlateCore",
			"TaraSimCore",     // ← the pure-sim module
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
		});
	}
}
