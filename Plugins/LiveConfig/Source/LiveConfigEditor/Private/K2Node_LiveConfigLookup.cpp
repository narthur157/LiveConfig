#include "K2Node_LiveConfigLookup.h"
#include "LiveConfigLib.h"
#include "LiveConfigGameSettings.h"
#include "EdGraphSchema_K2.h"
#include "KismetCompiler.h"
#include "K2Node_CallFunction.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"

#define LOCTEXT_NAMESPACE "K2Node_LiveConfigLookup"

FName UK2Node_LiveConfigLookup::PropertyPinName(TEXT("Property"));
FName UK2Node_LiveConfigLookup::ValuePinName(TEXT("Value"));

UK2Node_LiveConfigLookup::UK2Node_LiveConfigLookup()
{
}

void UK2Node_LiveConfigLookup::AllocateDefaultPins()
{
	// Clear existing pins to avoid duplicates during reconstruction
	Pins.Empty();

	// Add property pin
	UEdGraphPin* PropertyPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, FLiveConfigProperty::StaticStruct(), PropertyPinName);
	
	// Add value pin
	if (DefaultPinType.PinCategory != NAME_None)
	{
		CreatePin(EGPD_Output, DefaultPinType.PinCategory, DefaultPinType.PinSubCategory, ValuePinName);
	}
	else
	{
		// Default to double-precision float for UE5
		CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Real, UEdGraphSchema_K2::PC_Double, ValuePinName);
	}

	UpdateOutputPinType();
}

FText UK2Node_LiveConfigLookup::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FLiveConfigProperty SelectedProperty;
	if (UEdGraphPin* PropertyPin = GetPropertyPin())
	{
		FLiveConfigProperty::StaticStruct()->ImportText(*PropertyPin->DefaultValue, &SelectedProperty, nullptr, 0, nullptr, FLiveConfigProperty::StaticStruct()->GetName());
	}

	if (SelectedProperty.IsValid())
	{
		return FText::Format(LOCTEXT("NodeTitle_WithProperty", "Get Live Config: {0}"), FText::FromName(SelectedProperty.GetName()));
	}

	return LOCTEXT("NodeTitle", "Get Live Config Value");
}

FText UK2Node_LiveConfigLookup::GetTooltipText() const
{
	return LOCTEXT("NodeTooltip", "Get the value of a Live Config property. The output pin type will automatically update based on the selected property.");
}

FLinearColor UK2Node_LiveConfigLookup::GetNodeTitleColor() const
{
	return FLinearColor(0.0f, 0.5f, 1.0f);
}

FText UK2Node_LiveConfigLookup::GetMenuCategory() const
{
	return LOCTEXT("MenuCategory", "Live Config");
}

void UK2Node_LiveConfigLookup::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	FLiveConfigProperty SelectedProperty;
	if (UEdGraphPin* PropertyPin = GetPropertyPin())
	{
		FLiveConfigProperty::StaticStruct()->ImportText(*PropertyPin->DefaultValue, &SelectedProperty, nullptr, 0, nullptr, FLiveConfigProperty::StaticStruct()->GetName());
	}

	if (SelectedProperty.IsValid())
	{
		if (const ULiveConfigGameSettings* GameSettings = GetDefault<ULiveConfigGameSettings>())
		{
			if (!GameSettings->PropertyDefinitions.Contains(SelectedProperty))
			{
				ULiveConfigGameSettings* MutableSettings = GetMutableDefault<ULiveConfigGameSettings>();
				FLiveConfigPropertyDefinition NewDef;
				NewDef.PropertyName = SelectedProperty;
				
				// Try to guess the type from the output pin if it's connected or changed
				UEdGraphPin* ValuePin = GetValuePin();
				if (ValuePin)
				{
					if (ValuePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
					{
						NewDef.PropertyType = ELiveConfigPropertyType::Bool;
					}
					else if (ValuePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int)
					{
						NewDef.PropertyType = ELiveConfigPropertyType::Int;
					}
					else if (ValuePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Float || ValuePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Real)
					{
						NewDef.PropertyType = ELiveConfigPropertyType::Float;
					}
					else if (ValuePin->PinType.PinCategory == UEdGraphSchema_K2::PC_String)
					{
						NewDef.PropertyType = ELiveConfigPropertyType::String;
					}
				}

				MutableSettings->PropertyDefinitions.Add(SelectedProperty, NewDef);
				MutableSettings->SaveConfig();
				MutableSettings->UpdateDefaultConfigFile();

				if (ULiveConfigSystem* System = GEngine->GetEngineSubsystem<ULiveConfigSystem>())
				{
					System->RefreshFromSettings();
				}
			}
		}
	}

	if (Pin && Pin->PinName == PropertyPinName)
	{
		UpdateOutputPinType();
	}
}

void UK2Node_LiveConfigLookup::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		// Helper for setting pin type on spawned node
		auto CustomizeLambda = [](UEdGraphNode* NewNode, bool bIsTemplateNode, FEdGraphPinType PinType)
		{
			UK2Node_LiveConfigLookup* LookupNode = CastChecked<UK2Node_LiveConfigLookup>(NewNode);
			LookupNode->DefaultPinType = PinType;
			LookupNode->ReconstructNode();
		};

		// Define supported types to register with the action database
		struct FSupportedType
		{
			FName Category;
			FName SubCategory;
			FText DisplayName;
		};

		TArray<FSupportedType> SupportedTypes = {
			{ UEdGraphSchema_K2::PC_Boolean, NAME_None, LOCTEXT("TypeBool", "Boolean") },
			{ UEdGraphSchema_K2::PC_Int, NAME_None, LOCTEXT("TypeInt", "Integer") },
			{ UEdGraphSchema_K2::PC_Real, UEdGraphSchema_K2::PC_Double, LOCTEXT("TypeFloat", "Float") },
			{ UEdGraphSchema_K2::PC_String, NAME_None, LOCTEXT("TypeString", "String") }
		};

		for (const FSupportedType& TypeInfo : SupportedTypes)
		{
			UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(ActionKey);
			check(NodeSpawner != nullptr);

			FEdGraphPinType PinType;
			PinType.PinCategory = TypeInfo.Category;
			PinType.PinSubCategory = TypeInfo.SubCategory;

			NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(CustomizeLambda, PinType);

			NodeSpawner->DefaultMenuSignature.Category = GetMenuCategory();
			NodeSpawner->DefaultMenuSignature.MenuName = FText::Format(LOCTEXT("MenuName_WithType", "Get Live Config Value ({0})"), TypeInfo.DisplayName);
			NodeSpawner->DefaultMenuSignature.Tooltip = GetTooltipText();

			ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
		}
	}
}

void UK2Node_LiveConfigLookup::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	UEdGraphPin* PropertyPin = GetPropertyPin();
	UEdGraphPin* ValuePin = GetValuePin();

	if (!PropertyPin || !ValuePin)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("InternalError", "Internal error in UK2Node_LiveConfigLookup").ToString());
		BreakAllNodeLinks();
		return;
	}

	FLiveConfigProperty SelectedProperty;
	FLiveConfigProperty::StaticStruct()->ImportText(*PropertyPin->DefaultValue, &SelectedProperty, nullptr, 0, nullptr, FLiveConfigProperty::StaticStruct()->GetName());

	if (!SelectedProperty.IsValid())
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("InvalidProperty", "No property selected in UK2Node_LiveConfigLookup").ToString());
		BreakAllNodeLinks();
		return;
	}

	ELiveConfigPropertyType PropType = ELiveConfigPropertyType::Float;
	if (const ULiveConfigGameSettings* GameSettings = GetDefault<ULiveConfigGameSettings>())
	{
		if (const FLiveConfigPropertyDefinition* Def = GameSettings->PropertyDefinitions.Find(SelectedProperty))
		{
			PropType = Def->PropertyType;
		}
	}

	FName FunctionName;
	switch (PropType)
	{
	case ELiveConfigPropertyType::Bool:
		FunctionName = GET_FUNCTION_NAME_CHECKED(ULiveConfigLib, IsFeatureEnabled);
		break;
	case ELiveConfigPropertyType::Int:
		FunctionName = GET_FUNCTION_NAME_CHECKED(ULiveConfigLib, GetIntValue);
		break;
	case ELiveConfigPropertyType::Float:
		FunctionName = GET_FUNCTION_NAME_CHECKED(ULiveConfigLib, GetValue);
		break;
	case ELiveConfigPropertyType::String:
		FunctionName = GET_FUNCTION_NAME_CHECKED(ULiveConfigLib, GetStringValue);
		break;
	default:
		FunctionName = GET_FUNCTION_NAME_CHECKED(ULiveConfigLib, GetValue);
		break;
	}

	UK2Node_CallFunction* CallFunctionNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallFunctionNode->FunctionReference.SetExternalMember(FunctionName, ULiveConfigLib::StaticClass());
	CallFunctionNode->AllocateDefaultPins();

	UEdGraphPin* CallPropertyPin = CallFunctionNode->FindPinChecked(TEXT("Property"));
	UEdGraphPin* CallReturnPin = CallFunctionNode->GetReturnValuePin();

	// Move connections
	CompilerContext.MovePinLinksToIntermediate(*PropertyPin, *CallPropertyPin);
	CompilerContext.MovePinLinksToIntermediate(*ValuePin, *CallReturnPin);

	// If property pin has no connections, copy the default value
	if (CallPropertyPin->LinkedTo.Num() == 0)
	{
		CallPropertyPin->DefaultValue = PropertyPin->DefaultValue;
	}

	BreakAllNodeLinks();
}

UEdGraphPin* UK2Node_LiveConfigLookup::GetPropertyPin() const
{
	return FindPin(PropertyPinName);
}

UEdGraphPin* UK2Node_LiveConfigLookup::GetValuePin() const
{
	return FindPin(ValuePinName);
}

void UK2Node_LiveConfigLookup::UpdateOutputPinType()
{
	UEdGraphPin* PropertyPin = GetPropertyPin();
	UEdGraphPin* ValuePin = GetValuePin();

	if (!PropertyPin || !ValuePin)
	{
		return;
	}

	FLiveConfigProperty SelectedProperty;
	FLiveConfigProperty::StaticStruct()->ImportText(*PropertyPin->DefaultValue, &SelectedProperty, nullptr, 0, nullptr, FLiveConfigProperty::StaticStruct()->GetName());

	if (!SelectedProperty.IsValid())
	{
		// Fallback to default type if set
		if (DefaultPinType.PinCategory != NAME_None && ValuePin->PinType != DefaultPinType)
		{
			ValuePin->PinType = DefaultPinType;
			
			if (UEdGraph* Graph = GetGraph())
			{
				Graph->NotifyGraphChanged();
			}
		}
		return;
	}

	ELiveConfigPropertyType PropType = ELiveConfigPropertyType::Float;
	if (const ULiveConfigGameSettings* GameSettings = GetDefault<ULiveConfigGameSettings>())
	{
		if (const FLiveConfigPropertyDefinition* Def = GameSettings->PropertyDefinitions.Find(SelectedProperty))
		{
			PropType = Def->PropertyType;
		}
	}

	FEdGraphPinType NewType;
	NewType.PinCategory = UEdGraphSchema_K2::PC_Real; // Default to Real
	NewType.PinSubCategory = UEdGraphSchema_K2::PC_Double;

	switch (PropType)
	{
	case ELiveConfigPropertyType::Bool:
		NewType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
		NewType.PinSubCategory = NAME_None;
		break;
	case ELiveConfigPropertyType::Int:
		NewType.PinCategory = UEdGraphSchema_K2::PC_Int;
		NewType.PinSubCategory = NAME_None;
		break;
	case ELiveConfigPropertyType::Float:
		NewType.PinCategory = UEdGraphSchema_K2::PC_Real;
		NewType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
		break;
	case ELiveConfigPropertyType::String:
		NewType.PinCategory = UEdGraphSchema_K2::PC_String;
		NewType.PinSubCategory = NAME_None;
		break;
	}

	if (ValuePin->PinType != NewType)
	{
		ValuePin->PinType = NewType;
		
		// Notify the graph that the node has changed
		if (UEdGraph* Graph = GetGraph())
		{
			Graph->NotifyGraphChanged();
		}
	}
}

void UK2Node_LiveConfigLookup::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	if (Pin == GetValuePin() && Pin->LinkedTo.Num() > 0)
	{
		// Match the type of the first connection
		UEdGraphPin* OtherPin = Pin->LinkedTo[0];
		if (OtherPin)
		{
			Pin->PinType = OtherPin->PinType;
		}
	}
}

#undef LOCTEXT_NAMESPACE
