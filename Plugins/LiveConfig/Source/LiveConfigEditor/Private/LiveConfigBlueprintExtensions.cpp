#include "LiveConfigBlueprintExtensions.h"
#include "K2Node_CallFunction.h"
#include "K2Node_LiveConfigLookup.h"
#include "LiveConfigLib.h"
#include "LiveConfigEditorSettings.h"
#include "LiveConfigGameSettings.h"
#include "LiveConfigSystem.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "GraphEditorModule.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "ScopedTransaction.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "LiveConfigBlueprintExtensions"

void FLiveConfigBlueprintExtensions::Initialize()
{
	UE_LOG(LogLiveConfig, Log, TEXT("FLiveConfigBlueprintExtensions::Initialize() called"));

	if (!UToolMenus::IsToolMenuUIEnabled())
	{
		UE_LOG(LogLiveConfig, Warning, TEXT("ToolMenu UI is NOT enabled. Skipping menu registration."));
		return;
	}

	UE_LOG(LogLiveConfig, Log, TEXT("Registering startup callback for ToolMenus"));
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateStatic(&FLiveConfigBlueprintExtensions::RegisterMenus));
	
	// Also try to register immediately if UToolMenus is already ready
	if (UToolMenus::Get())
	{
		UE_LOG(LogLiveConfig, Log, TEXT("UToolMenus already exists, calling RegisterMenus immediately"));
		RegisterMenus();
	}
}

void FLiveConfigBlueprintExtensions::RegisterMenus()
{
	UE_LOG(LogLiveConfig, Log, TEXT("Registering menus..."));
	FToolMenuOwnerScoped OwnerScoped("LiveConfig");

	TArray<FName> MenuNames = { 
		"GraphEditor.PinContextMenu", 
		"EdGraphSchema_K2.PinContextMenu",
		"EdGraphSchema.ContextMenu"
	};

	for (const FName& MenuName : MenuNames)
	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu(MenuName);
		if (Menu)
		{
			UE_LOG(LogLiveConfig, Log, TEXT("Extending menu: %s"), *MenuName.ToString());
			
			FToolMenuSection& Section = Menu->FindOrAddSection("EdGraphSchemaPinActions");
			
			UE_LOG(LogLiveConfig, Log, TEXT("Adding entries to section: EdGraphSchemaPinActions in menu: %s"), *MenuName.ToString());

			Section.AddDynamicEntry("LiveConfigPromote", FNewToolMenuSectionDelegate::CreateLambda([MenuName](FToolMenuSection& InSection)
			{
				UE_LOG(LogLiveConfig, Log, TEXT("Dynamic entry callback for section: %s in menu: %s"), *InSection.Name.ToString(), *MenuName.ToString());
				AddPromoteToLiveConfigMenu(InSection);
			}));
		}
		else
		{
			UE_LOG(LogLiveConfig, Warning, TEXT("Could not find menu to extend: %s"), *MenuName.ToString());
		}
	}
}

void FLiveConfigBlueprintExtensions::Shutdown()
{
	UToolMenus::UnregisterOwner("LiveConfig");
}

void FLiveConfigBlueprintExtensions::AddPromoteToLiveConfigMenu(FToolMenuSection& InSection)
{
	UE_LOG(LogLiveConfig, Log, TEXT("AddPromoteToLiveConfigMenu called for section: %s"), *InSection.Name.ToString());

	UGraphNodeContextMenuContext* Context = InSection.FindContext<UGraphNodeContextMenuContext>();
	if (!Context)
	{
		UE_LOG(LogLiveConfig, Warning, TEXT("No context found in section: %s"), *InSection.Name.ToString());
		return;
	}

	if (!Context->Pin)
	{
		UE_LOG(LogLiveConfig, Warning, TEXT("No pin found in context for section: %s"), *InSection.Name.ToString());
		return;
	}

	UEdGraphPin* Pin = const_cast<UEdGraphPin*>(Context->Pin);
	UE_LOG(LogLiveConfig, Log, TEXT("Found pin: %s (Type: %s, Direction: %d)"), 
		*Pin->GetName(), *Pin->PinType.PinCategory.ToString(), (int32)Pin->Direction);

	if (CanPromotePinToLiveConfig(Pin))
	{
		UE_LOG(LogLiveConfig, Log, TEXT("Pin %s is eligible for promotion. Adding menu entry."), *Pin->GetName());
		InSection.AddMenuEntry(
			"PromoteToLiveConfig",
			LOCTEXT("PromoteToLiveConfig", "Promote to Live Config Property"),
			LOCTEXT("PromoteToLiveConfigTooltip", "Create a new Live Config property from this pin value"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateStatic(&FLiveConfigBlueprintExtensions::PromotePinToLiveConfig, Pin))
		);
	}
	else
	{
		UE_LOG(LogLiveConfig, Log, TEXT("Pin %s is NOT eligible for promotion."), *Pin->GetName());
	}
}

void FLiveConfigBlueprintExtensions::AddPromoteToLiveConfigMenu(FMenuBuilder& MenuBuilder, UEdGraphPin* Pin)
{
	if (CanPromotePinToLiveConfig(Pin))
	{
		UE_LOG(LogLiveConfig, Log, TEXT("FExtender: Adding menu entry for pin: %s"), *Pin->GetName());
		MenuBuilder.BeginSection("LiveConfig", LOCTEXT("LiveConfigSection", "Live Config"));
		MenuBuilder.AddMenuEntry(
			LOCTEXT("PromoteToLiveConfig", "Promote to Live Config Property"),
			LOCTEXT("PromoteToLiveConfigTooltip", "Create a new Live Config property from this pin value"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateStatic(&FLiveConfigBlueprintExtensions::PromotePinToLiveConfig, Pin))
		);
		MenuBuilder.EndSection();
	}
}

TSharedRef<FExtender> FLiveConfigBlueprintExtensions::OnExtendPinMenu(const TSharedRef<FUICommandList> CommandList, const UEdGraph* Graph, const UEdGraphNode* Node, const UEdGraphPin* Pin, bool bReadOnly)
{
	UE_LOG(LogLiveConfig, Log, TEXT("OnExtendPinMenu called for pin: %s"), Pin ? *Pin->GetName() : TEXT("None"));
	TSharedRef<FExtender> Extender = MakeShareable(new FExtender());

	if (Pin)
	{
		Extender->AddMenuExtension(
			"EdGraphSchemaPinActions",
			EExtensionHook::After,
			CommandList,
			FMenuExtensionDelegate::CreateStatic(&FLiveConfigBlueprintExtensions::AddPromoteToLiveConfigMenu, const_cast<UEdGraphPin*>(Pin))
		);
	}

	return Extender;
}

void FLiveConfigBlueprintExtensions::PromotePinToLiveConfig(UEdGraphPin* Pin)
{
	if (!Pin || !Pin->GetOwningNode())
	{
		return;
	}

	UEdGraph* Graph = Pin->GetOwningNode()->GetGraph();
	if (!Graph)
	{
		return;
	}

	// Get pin information
	// FName PropertyType = GetPinTypeName(Pin); // No longer needed as we use Pin->PinType directly

	// Create the node
	const FScopedTransaction Transaction(LOCTEXT("PromoteToLiveConfigTransaction", "Promote to Live Config"));
	Graph->Modify();

	UK2Node_LiveConfigLookup* NewNode = NewObject<UK2Node_LiveConfigLookup>(Graph);
	
	NewNode->DefaultPinType = Pin->PinType;
	NewNode->AllocateDefaultPins();
	
	Graph->AddNode(NewNode, false, false);

	// Position the node near the pin
	if (UEdGraphNode* OwningNode = Pin->GetOwningNode())
	{
		NewNode->NodePosX = OwningNode->NodePosX - 400; // Position to the left of the input pin
		NewNode->NodePosY = OwningNode->NodePosY;
	}

	// Set property name on the Property pin
	UEdGraphPin* PropertyPin = NewNode->GetPropertyPin();

	if (PropertyPin)
	{
		// Use the pin name as a suggestion for the property name if it's not a generic name
		FString PropertyNameSuggestion = Pin->GetName();
		PropertyPin->DefaultValue = FString::Printf(TEXT("(PropertyName=\"%s\")"), *PropertyNameSuggestion);
		
		// Add this property to the game settings if it doesn't exist
		ULiveConfigSystem* System = ULiveConfigSystem::Get();
		FLiveConfigProperty PropertyKey;
		PropertyKey.PropertyName = FName(*PropertyNameSuggestion);
		if (!System->PropertyDefinitions.Contains(PropertyKey))
		{
			FLiveConfigPropertyDefinition NewDef;
			NewDef.PropertyName = PropertyKey;
			NewDef.Description = FString::Printf(TEXT("Promoted from pin %s in %s"), *Pin->GetName(), *Graph->GetName());
			
			// Set correct type in definition
			if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
			{
				NewDef.PropertyType = ELiveConfigPropertyType::Bool;
			}
			else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int64)
			{
				NewDef.PropertyType = ELiveConfigPropertyType::Int;
			}
			else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Float || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Real || Pin->PinType.PinCategory == TEXT("double"))
			{
				NewDef.PropertyType = ELiveConfigPropertyType::Float;
			}
			else
			{
				NewDef.PropertyType = ELiveConfigPropertyType::String;
			}
			
			System->PropertyDefinitions.Add(PropertyKey, NewDef);
			System->SaveConfig();
			System->TryUpdateDefaultConfigFile();
		}
	}

	// Connect the output pin to the original pin
	UEdGraphPin* ValuePin = NewNode->GetValuePin();
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	
	// If the pin was connected to something, break those links first
	if (Pin->LinkedTo.Num() > 0)
	{
		Pin->BreakAllPinLinks();
	}

	if (ValuePin)
	{
		Schema->TryCreateConnection(ValuePin, Pin);
	}

	// Notify that the node was created
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
	if (Blueprint)
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	}
}

bool FLiveConfigBlueprintExtensions::CanPromotePinToLiveConfig(UEdGraphPin* Pin)
{
	if (!Pin)
	{
		return false;
	}

	// Can only promote input pins
	if (Pin->Direction != EGPD_Input)
	{
		return false;
	}

	// Check if pin type is supported
	if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Double ||
		Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Real ||
		Pin->PinType.PinCategory == TEXT("float") ||
		Pin->PinType.PinCategory == TEXT("double") ||
		Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int ||
		Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int64 ||
		Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean ||
		Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_String ||
		Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Name ||
		Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Text)
	{
		return true;
	}

	return false;
}

FName FLiveConfigBlueprintExtensions::GetPinTypeName(const UEdGraphPin* Pin)
{
	if (!Pin)
	{
		return NAME_None;
	}

	if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Double ||
		Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Real ||
		Pin->PinType.PinCategory == TEXT("float") ||
		Pin->PinType.PinCategory == TEXT("double"))
	{
		return TEXT("Float");
	}
	else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int64)
	{
		return TEXT("Int");
	}
	else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
	{
		return TEXT("Bool");
	}
	else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_String)
	{
		return TEXT("String");
	}
	else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Name)
	{
		return TEXT("String");
	}
	else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Text)
	{
		return TEXT("String");
	}

	return TEXT("String");
}

#undef LOCTEXT_NAMESPACE
