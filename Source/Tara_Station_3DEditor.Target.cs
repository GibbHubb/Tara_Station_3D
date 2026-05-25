using UnrealBuildTool;
using System.Collections.Generic;

public class Tara_Station_3DEditorTarget : TargetRules
{
	public Tara_Station_3DEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.AddRange(new string[] { "TaraGame", "TaraSimCore" });
	}
}
