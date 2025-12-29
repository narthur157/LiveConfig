#pragma once
#include "KismetPins/SGraphPinStructInstance.h"

class SLiveConfigPropertyDefinitionPin : public SGraphPinStructInstance
{
public:
	SLATE_BEGIN_ARGS(SLiveConfigPropertyDefinitionPin) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);

protected:
	// This is the most important function.
	// It returns the widget to display when the pin is not connected.
	virtual TSharedRef<SWidget> GetDefaultValueWidget() override;

	// You'll also need to handle setting the property's value from your widget.
	// This is just an example of what you might have.
	void OnDropdownSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectType);
};
