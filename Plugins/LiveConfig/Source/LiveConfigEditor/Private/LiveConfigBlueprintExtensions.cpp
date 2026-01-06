#include "LiveConfigBlueprintExtensions.h"
#include "K2Node_CallFunction.h"
#include "LiveConfigLib.h"
#include "LiveConfigEditorSettings.h"
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
	UE_LOG(LogTemp, Log, TEXT("[LiveConfig] FLiveConfigBlueprintExtensions::Initialize() called"));

	if (!UToolMenus::IsToolMenuUIEnabled())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LiveConfig] ToolMenu UI is NOT enabled. Skipping menu registration."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[LiveConfig] Registering startup callback for ToolMenus"));
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateStatic(&FLiveConfigBlueprintExtensions::RegisterMenus));
	
	// Also try to register immediately if UToolMenus is already ready
	if (UToolMenus::Get())
	{
		UE_LOG(LogTemp, Log, TEXT("[LiveConfig] UToolMenus already exists, calling RegisterMenus immediately"));
		RegisterMenus();
	}
}

void FLiveConfigBlueprintExtensions::RegisterMenus()
{
	UE_LOG(LogTemp, Log, TEXT("[LiveConfig] Registering menus..."));
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
			UE_LOG(LogTemp, Log, TEXT("[LiveConfig] Extending menu: %s"), *MenuName.ToString());
			
			FToolMenuSection& Section = Menu->FindOrAddSection("EdGraphSchemaPinActions");
			
			UE_LOG(LogTemp, Log, TEXT("[LiveConfig] Adding entries to section: EdGraphSchemaPinActions in menu: %s"), *MenuName.ToString());

			// Add a static entry for testing
			Section.AddMenuEntry(
				"LiveConfigTestStatic",
				LOCTEXT("LiveConfigTestStatic", "Live Config Test (Static)"),
				LOCTEXT("LiveConfigTestStaticTooltip", "Static test entry"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([](){ UE_LOG(LogTemp, Log, TEXT("[LiveConfig] Static test menu clicked")); }))
			);

			Section.AddDynamicEntry("LiveConfigPromote", FNewToolMenuSectionDelegate::CreateLambda([MenuName](FToolMenuSection& InSection)
			{
				UE_LOG(LogTemp, Log, TEXT("[LiveConfig] Dynamic entry callback for section: %s in menu: %s"), *InSection.Name.ToString(), *MenuName.ToString());
				AddPromoteToLiveConfigMenu(InSection);
			}));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[LiveConfig] Could not find menu to extend: %s"), *MenuName.ToString());
		}
	}
}

void FLiveConfigBlueprintExtensions::Shutdown()
{
	UToolMenus::UnregisterOwner("LiveConfig");
}

void FLiveConfigBlueprintExtensions::AddPromoteToLiveConfigMenu(FToolMenuSection& InSection)
{
	UE_LOG(LogTemp, Log, TEXT("[LiveConfig] AddPromoteToLiveConfigMenu called for section: %s"), *InSection.Name.ToString());

	UGraphNodeContextMenuContext* Context = InSection.FindContext<UGraphNodeContextMenuContext>();
	if (!Context)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LiveConfig] No context found in section: %s"), *InSection.Name.ToString());
		return;
	}

	if (!Context->Pin)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LiveConfig] No pin found in context for section: %s"), *InSection.Name.ToString());
		return;
	}

	UEdGraphPin* Pin = const_cast<UEdGraphPin*>(Context->Pin);
	UE_LOG(LogTemp, Log, TEXT("[LiveConfig] Found pin: %s (Type: %s, Direction: %d)"), 
		*Pin->GetName(), *Pin->PinType.PinCategory.ToString(), (int32)Pin->Direction);

	if (CanPromotePinToLiveConfig(Pin))
	{
		UE_LOG(LogTemp, Log, TEXT("[LiveConfig] Pin %s is eligible for promotion. Adding menu entry."), *Pin->GetName());
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
		UE_LOG(LogTemp, Log, TEXT("[LiveConfig] Pin %s is NOT eligible for promotion."), *Pin->GetName());
	}
}

void FLiveConfigBlueprintExtensions::AddPromoteToLiveConfigMenu(FMenuBuilder& MenuBuilder, UEdGraphPin* Pin)
{
	if (CanPromotePinToLiveConfig(Pin))
	{
		UE_LOG(LogTemp, Log, TEXT("[LiveConfig] FExtender: Adding menu entry for pin: %s"), *Pin->GetName());
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
	UE_LOG(LogTemp, Log, TEXT("[LiveConfig] OnExtendPinMenu called for pin: %s"), Pin ? *Pin->GetName() : TEXT("None"));
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
	FName PropertyType = GetPinTypeName(Pin);
	FString DefaultValue = GetPinDefaultValueString(Pin);

	// Create the node
	const FScopedTransaction Transaction(LOCTEXT("PromoteToLiveConfigTransaction", "Promote to Live Config"));
	Graph->Modify();

	UK2Node_CallFunction* NewNode = NewObject<UK2Node_CallFunction>(Graph);

	// Determine which function to call based on type
	FName FunctionName = NAME_None;
	if (PropertyType == TEXT("Float"))
	{
		FunctionName = GET_FUNCTION_NAME_CHECKED(ULiveConfigLib, GetValue);
	}
	else if (PropertyType == TEXT("Int"))
	{
		FunctionName = GET_FUNCTION_NAME_CHECKED(ULiveConfigLib, GetIntValue);
	}
	else if (PropertyType == TEXT("Bool"))
	{
		FunctionName = GET_FUNCTION_NAME_CHECKED(ULiveConfigLib, IsFeatureEnabled);
	}
	else
	{
		FunctionName = GET_FUNCTION_NAME_CHECKED(ULiveConfigLib, GetStringValue);
	}

	NewNode->FunctionReference.SetExternalMember(FunctionName, ULiveConfigLib::StaticClass());
	NewNode->AllocateDefaultPins();
	
	Graph->AddNode(NewNode, false, false);

	// Position the node near the pin
	if (UEdGraphNode* OwningNode = Pin->GetOwningNode())
	{
		NewNode->NodePosX = OwningNode->NodePosX - 400; // Position to the left of the input pin
		NewNode->NodePosY = OwningNode->NodePosY;
	}

	// Set property name on the Property pin
	UEdGraphPin* PropertyPin = NewNode->FindPin(TEXT("Property"));

	if (PropertyPin)
	{
		PropertyPin->DefaultValue = TEXT("(PropertyName=\"None\")");
	}

	// Set default value on the DefaultValue/bDefault/DefaultString pin
	UEdGraphPin* DefaultValuePin = NewNode->FindPin(TEXT("DefaultValue"));
	if (!DefaultValuePin)
	{
		DefaultValuePin = NewNode->FindPin(TEXT("bDefault"));
	}
	if (!DefaultValuePin)
	{
		DefaultValuePin = NewNode->FindPin(TEXT("DefaultString"));
	}

	if (DefaultValuePin)
	{
		DefaultValuePin->DefaultValue = DefaultValue;
	}

	// Connect the output pin to the original pin
	UEdGraphPin* ValuePin = NewNode->GetReturnValuePin();
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

FString FLiveConfigBlueprintExtensions::GetPinDefaultValueString(const UEdGraphPin* Pin)
{
	if (!Pin)
	{
		return FString();
	}

	// Return the current default value or a sensible default
	if (!Pin->DefaultValue.IsEmpty())
	{
		return Pin->DefaultValue;
	}
	else if (!Pin->AutogeneratedDefaultValue.IsEmpty())
	{
		return Pin->AutogeneratedDefaultValue;
	}

	// Provide type-appropriate defaults
	if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Double ||
		Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Real ||
		Pin->PinType.PinCategory == TEXT("float") ||
		Pin->PinType.PinCategory == TEXT("double"))
	{
		return TEXT("0.0");
	}
	else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int64)
	{
		return TEXT("0");
	}
	else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
	{
		return TEXT("false");
	}

	return FString();
}

#undef LOCTEXT_NAMESPACE
