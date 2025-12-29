#include "SLiveConfigPropertyDefinitionPin.h"

#include "Widgets/Input/STextComboBox.h"

void SLiveConfigPropertyDefinitionPin::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
}

TSharedRef<SWidget> SLiveConfigPropertyDefinitionPin::GetDefaultValueWidget()
{
	// Normally, you'd populate this from some asset or data
	static TArray<TSharedPtr<FString>> Options;
	Options.Add(MakeShareable(new FString(TEXT("Custom Option 1"))));
	Options.Add(MakeShareable(new FString(TEXT("Custom Option 2"))));

	// Create your custom widget. This could be anything.
	// We'll use a simple text combo box as an example.
	return SNew(STextComboBox)
		.OptionsSource(&Options)
		.OnSelectionChanged(this, &SLiveConfigPropertyDefinitionPin::OnDropdownSelectionChanged);
        
	// Look at SGraphPinGameplayTag in the engine source
	// for a real, complex example.
}

void SLiveConfigPropertyDefinitionPin::OnDropdownSelectionChanged(TSharedPtr<FString> NewSelection,
	ESelectInfo::Type SelectType)
{
	// When the user picks a new value, you must set the pin's default value.
	if (GraphPinObj && NewSelection.IsValid())
	{
		// Set the property's default value on the pin.
		// This requires parsing the string and setting the property.
		// This part is highly dependent on your data type.
		// For an FString, it's easy:
		// GraphPinObj->GetSchema()->SetPinDefaultValue(GraphPinObj, *NewSelection);
        
		// For a struct, you'd have to convert the string to the struct's text representation.
		// FYourStruct StructValue;
		// StructValue.InitializeFromString(*NewSelection);
		// FString StructAsText;
		// FYourStruct::StaticStruct()->ExportText(StructAsText, &StructValue, ...);
		// GraphPinObj->GetSchema()->SetPinDefaultValue(GraphPinObj, StructAsText);
	}
}
