// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#pragma once

#include "CoreMinimal.h"
#include "Framework/MultiBox/MultiBoxExtender.h"

class UEdGraphPin;
class FMenuBuilder;
class SGraphPanel;

/**
 * Helper class to extend blueprint context menus with Live Config actions
 */
class FLiveConfigBlueprintExtensions
{
public:
	/** Initialize the blueprint extensions */
	static void Initialize();

	/** Shutdown the blueprint extensions */
	static void Shutdown();

	/** FExtender callback for pin context menu */
	static TSharedRef<FExtender> OnExtendPinMenu(const TSharedRef<FUICommandList> CommandList, const UEdGraph* Graph, const UEdGraphNode* Node, const UEdGraphPin* Pin, bool bReadOnly);

private:
	/** Internal function to register menus */
	static void RegisterMenus();

	/** Add "Promote to Live Config" menu entry (UToolMenus version) */
	static void AddPromoteToLiveConfigMenu(struct FToolMenuSection& InSection);

	/** Add "Promote to Live Config" menu entry (FExtender version) */
	static void AddPromoteToLiveConfigMenu(FMenuBuilder& MenuBuilder, UEdGraphPin* Pin);

	/** Execute the promote to live config action */
	static void PromotePinToLiveConfig(UEdGraphPin* Pin);

	/** Check if a pin can be promoted to Live Config */
	static bool CanPromotePinToLiveConfig(UEdGraphPin* Pin);

	/** Get the type name for a pin */
	static FName GetPinTypeName(const UEdGraphPin* Pin);
};
