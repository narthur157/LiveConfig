// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#include "Blueprint/K2Node_LiveConfigLookup.h"
#include "LiveConfigLib.h"
#include "LiveConfigSettings.h"
#include "EdGraphSchema_K2.h"
#include "KismetCompiler.h"
#include "K2Node_CallFunction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "LiveConfigTypes.h"

#define LOCTEXT_NAMESPACE "K2Node_LiveConfigLookup"

FName UK2Node_LiveConfigLookup::PropertyPinName(TEXT("Property"));
FName UK2Node_LiveConfigLookup::ValuePinName(TEXT("Value"));

/**
 * property type is determined by property pin literal if possible, else value pin connection. If neither, type should always be wildcard
 */

UK2Node_LiveConfigLookup::UK2Node_LiveConfigLookup()
{
}

void UK2Node_LiveConfigLookup::AllocateDefaultPins()
{
	// Add property pin
	if (!FindPin(PropertyPinName))
	{
		CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, FLiveConfigProperty::StaticStruct(), PropertyPinName);
	}
	
	// Add value pin
	if (!FindPin(ValuePinName))
	{
		// Default to wildcard
		CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, ValuePinName);
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

	return LOCTEXT("NodeTitle", "Get Live Config");
}

FText UK2Node_LiveConfigLookup::GetTooltipText() const
{
	return LOCTEXT("NodeTooltip", "Get the value of a Live Config property. The output pin type will automatically update based on the selected property or what it's connected to.");
}

FLinearColor UK2Node_LiveConfigLookup::GetNodeTitleColor() const
{
	return FLinearColor(0.5f, 1.0f, .5f);
}

FText UK2Node_LiveConfigLookup::GetMenuCategory() const
{
	return LOCTEXT("MenuCategory", "Live Config");
}

void UK2Node_LiveConfigLookup::PinDefaultValueChanged(UEdGraphPin* Pin)
{
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
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(ActionKey);
		check(NodeSpawner != nullptr);

		NodeSpawner->DefaultMenuSignature.Category = GetMenuCategory();
		NodeSpawner->DefaultMenuSignature.MenuName = LOCTEXT("MenuName", "Get Live Config");
		NodeSpawner->DefaultMenuSignature.Tooltip = GetTooltipText();

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
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

	bool bInputIsConnected = PropertyPin->LinkedTo.Num() > 0;

	if (!SelectedProperty.IsValid() && !bInputIsConnected)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("InvalidProperty", "No property selected in UK2Node_LiveConfigLookup").ToString());
		BreakAllNodeLinks();
		return;
	}

	// Determine the property type based on the value pin's current type.
	ELiveConfigPropertyType PropType = ELiveConfigPropertyType::Float;
	
	if (ValuePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
	{
		PropType = ELiveConfigPropertyType::Bool;
	}
	else if (ValuePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int)
	{
		PropType = ELiveConfigPropertyType::Int;
	}
	else if (ValuePin->PinType.PinCategory == UEdGraphSchema_K2::PC_String)
	{
		PropType = ELiveConfigPropertyType::String;
	}
	else if (ValuePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
	{
		PropType = ELiveConfigPropertyType::Struct;
	}
	else if (ValuePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("WildcardError", "The output pin of 'Get Live Config' must be connected to a pin with a concrete type, or a property must be selected.").ToString());
		BreakAllNodeLinks();
		return;
	}
	
	// If we have a valid definition, we can do a sanity check.
	FLiveConfigPropertyDefinition Def = ULiveConfigLib::GetLiveConfigPropertyDefinition(SelectedProperty);
	if (Def.IsValid() && PropType != Def.PropertyType)
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("PropertyName"), FText::FromString(SelectedProperty.ToString()));
		Args.Add(TEXT("ExpectedType"), UEnum::GetDisplayValueAsText(Def.PropertyType));
		Args.Add(TEXT("ActualType"), UEnum::GetDisplayValueAsText(PropType));

		CompilerContext.MessageLog.Warning(*FText::Format(LOCTEXT("PropertyTypeMismatch", "Property type mismatch for '{PropertyName}'. Expected {ExpectedType}, but node is configured as {ActualType}."), Args).ToString());
	}

	FName FunctionName;
	switch (PropType)
	{
	case ELiveConfigPropertyType::Bool:
		FunctionName = GET_FUNCTION_NAME_CHECKED(ULiveConfigLib, GetBoolValue);
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
	case ELiveConfigPropertyType::Struct:
		FunctionName = GET_FUNCTION_NAME_CHECKED(ULiveConfigLib, GetStructValue);
		break;
	default:
		FunctionName = GET_FUNCTION_NAME_CHECKED(ULiveConfigLib, GetValue);
		break;
	}

	UK2Node_CallFunction* CallFunctionNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallFunctionNode->FunctionReference.SetExternalMember(FunctionName, ULiveConfigLib::StaticClass());
	CallFunctionNode->AllocateDefaultPins();

	UEdGraphPin* CallPropertyPin = CallFunctionNode->FindPinChecked(TEXT("Property"));
	
	if (PropType == ELiveConfigPropertyType::Struct)
	{
		UEdGraphPin* CallOutStructPin = CallFunctionNode->FindPinChecked(TEXT("OutStruct"));
		CallOutStructPin->PinType = ValuePin->PinType;
		CompilerContext.MovePinLinksToIntermediate(*ValuePin, *CallOutStructPin);
	}
	else
	{
		UEdGraphPin* CallReturnPin = CallFunctionNode->GetReturnValuePin();
		CompilerContext.MovePinLinksToIntermediate(*ValuePin, *CallReturnPin);
	}

	// Move connections
	CompilerContext.MovePinLinksToIntermediate(*PropertyPin, *CallPropertyPin);

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

	FEdGraphPinType NewType;
	NewType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;

	bool bTypeSet = false;

	// Try to get type from literal (default value)
	FLiveConfigProperty PropertyPinLiteral;
	FLiveConfigProperty::StaticStruct()->ImportText(*PropertyPin->DefaultValue, &PropertyPinLiteral, nullptr, 0, nullptr, FLiveConfigProperty::StaticStruct()->GetName());

	if (PropertyPinLiteral.IsValid())
	{
		FLiveConfigPropertyDefinition Def = ULiveConfigLib::GetLiveConfigPropertyDefinition(PropertyPinLiteral);
		if (Def.IsValid())
		{
			bTypeSet = true;
			switch (Def.PropertyType)
			{
			case ELiveConfigPropertyType::Bool:
				NewType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
				break;
			case ELiveConfigPropertyType::Int:
				NewType.PinCategory = UEdGraphSchema_K2::PC_Int;
				break;
			case ELiveConfigPropertyType::Float:
				NewType.PinCategory = UEdGraphSchema_K2::PC_Real;
				NewType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
				break;
			case ELiveConfigPropertyType::String:
				NewType.PinCategory = UEdGraphSchema_K2::PC_String;
				break;
			case ELiveConfigPropertyType::Struct:
				NewType.PinCategory = UEdGraphSchema_K2::PC_Struct;
				
				// If you hit an ensure here, it's because there is a type collision and a struct probably needs to be renamed
				NewType.PinSubCategoryObject = FindFirstObject<UScriptStruct>(*Def.Value, EFindFirstObjectOptions::EnsureIfAmbiguous);
				if (!NewType.PinSubCategoryObject.IsValid() && !Def.Value.IsEmpty())
				{
					UE_LOG(LogLiveConfig, Error, TEXT("Failed to find struct for property %s with struct type %s, this could be due to an unloaded blueprint struct"), *Def.PropertyName.ToString(), *Def.Value);
				}
				break;
			default:
				bTypeSet = false;
				break;
			}
		}
	}

	// If no property selected, try to get type from connected pin
	if (!bTypeSet && ValuePin->LinkedTo.Num() > 0)
	{
		NewType = ValuePin->LinkedTo[0]->PinType;
		bTypeSet = true;
	}

	// If neither, NewType is already initialized to Wildcard

	if (ValuePin->PinType != NewType)
	{
		ValuePin->PinType = NewType;
		
		// Notify the graph that the node has changed
		if (UEdGraph* Graph = GetGraph())
		{
			Graph->NotifyGraphChanged();
		}

		// Reconstruct node to ensure all internal state is updated when type changes
		// But don't do it if we are already in the middle of a reconstruction or connection event that will trigger one
		if (!HasAnyFlags(RF_NeedPostLoad) && !GIsTransacting)
		{
			if (UEdGraph* Graph = GetGraph())
			{
				if (const UEdGraphSchema* Schema = Graph->GetSchema())
				{
					Schema->ReconstructNode(*this);
				}
			}
		}
	}
}

void UK2Node_LiveConfigLookup::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	if (Pin == GetValuePin())
	{
		UpdateOutputPinType();
	}
	else if (Pin == GetPropertyPin())
	{
		UpdateOutputPinType();
	}
}

bool UK2Node_LiveConfigLookup::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutConnectionMessage) const
{
	if (MyPin == GetValuePin() && MyPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
	{
		return false; // Wildcard can connect to anything
	}

	return Super::IsConnectionDisallowed(MyPin, OtherPin, OutConnectionMessage);
}

void UK2Node_LiveConfigLookup::PostReconstructNode()
{
	Super::PostReconstructNode();

	UEdGraphPin* PropertyPin = GetPropertyPin();
	UEdGraphPin* ValuePin = GetValuePin();

	if (PropertyPin && ValuePin)
	{
		FEdGraphPinType NewType;
		NewType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;

		bool bTypeSet = false;

		// 1. Try to get type from selected property (Property Pin Literal)
		FLiveConfigProperty SelectedProperty;
		FLiveConfigProperty::StaticStruct()->ImportText(*PropertyPin->DefaultValue, &SelectedProperty, nullptr, 0, nullptr, FLiveConfigProperty::StaticStruct()->GetName());

		if (SelectedProperty.IsValid())
		{
			FLiveConfigPropertyDefinition Def = ULiveConfigLib::GetLiveConfigPropertyDefinition(SelectedProperty);
			if (Def.IsValid())
			{
				bTypeSet = true;
				switch (Def.PropertyType)
				{
				case ELiveConfigPropertyType::Bool:
					NewType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
					break;
				case ELiveConfigPropertyType::Int:
					NewType.PinCategory = UEdGraphSchema_K2::PC_Int;
					break;
				case ELiveConfigPropertyType::Float:
					NewType.PinCategory = UEdGraphSchema_K2::PC_Real;
					NewType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
					break;
				case ELiveConfigPropertyType::String:
					NewType.PinCategory = UEdGraphSchema_K2::PC_String;
					break;
				case ELiveConfigPropertyType::Struct:
					NewType.PinCategory = UEdGraphSchema_K2::PC_Struct;
					NewType.PinSubCategoryObject = FindFirstObject<UScriptStruct>(*Def.Value, EFindFirstObjectOptions::EnsureIfAmbiguous);
					break;
				default:
					bTypeSet = false;
					break;
				}
			}
		}

		// 2. If no property selected, try to get type from connected pin
		if (!bTypeSet && ValuePin->LinkedTo.Num() > 0)
		{
			NewType = ValuePin->LinkedTo[0]->PinType;
			bTypeSet = true;
		}

		// 3. If neither, NewType is already initialized to Wildcard

		if (ValuePin->PinType != NewType)
		{
			ValuePin->PinType = NewType;
		}
	}
}

void UK2Node_LiveConfigLookup::ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const
{
	Super::ValidateNodeDuringCompilation(MessageLog);

	UEdGraphPin* PropertyPin = GetPropertyPin();
	if (PropertyPin && PropertyPin->LinkedTo.Num() == 0)
	{
		FLiveConfigProperty SelectedProperty;
		FLiveConfigProperty::StaticStruct()->ImportText(*PropertyPin->DefaultValue, &SelectedProperty, nullptr, 0, nullptr, FLiveConfigProperty::StaticStruct()->GetName());

		if (SelectedProperty.IsValid())
		{
			FLiveConfigPropertyDefinition Def = ULiveConfigLib::GetLiveConfigPropertyDefinition(SelectedProperty);
			if (!Def.IsValid())
			{
				MessageLog.Warning(*FText::Format(LOCTEXT("PropertyNotFoundWarning", "Live Config property '{0}' not found. @@"), FText::FromName(SelectedProperty.GetName())).ToString(), this);
			}
			else if (Def.IsDeprecated())
			{
				MessageLog.Warning(*FText::Format(LOCTEXT("PropertyDeprecatedWarning", "Live Config property '{0}' is deprecated. @@"), FText::FromName(SelectedProperty.GetName())).ToString(), this);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE

