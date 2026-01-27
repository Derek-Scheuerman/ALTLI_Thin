// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ALTLI_Thin : ModuleRules
{
	public ALTLI_Thin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"ALTLI_Thin",
			"ALTLI_Thin/Variant_Horror",
			"ALTLI_Thin/Variant_Horror/UI",
			"ALTLI_Thin/Variant_Shooter",
			"ALTLI_Thin/Variant_Shooter/AI",
			"ALTLI_Thin/Variant_Shooter/UI",
			"ALTLI_Thin/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
