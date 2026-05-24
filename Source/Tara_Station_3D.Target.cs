using UnrealBuildTool;
using System.Collections.Generic;

public class Tara_Station_3DTarget : TargetRules
{
	public Tara_Station_3DTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V4;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.AddRange(new string[] { "TaraGame", "TaraSimCore" });
	}
}
