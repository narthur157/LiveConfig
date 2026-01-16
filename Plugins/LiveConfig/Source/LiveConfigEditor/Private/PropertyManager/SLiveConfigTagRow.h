#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "LiveConfigSystem.h"

class SLiveConfigTagRow : public SCompoundWidget
{
	SLATE_DECLARE_WIDGET(SLiveConfigTagRow, SCompoundWidget);
public:
	SLATE_BEGIN_ARGS(SLiveConfigTagRow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FLiveConfigPropertyDefinition> InItem, int32 InIndex, FSimpleDelegate InOnChanged, TFunction<FSlateColor(FName)> InGetTagColor);

private:
	TSharedPtr<FLiveConfigPropertyDefinition> Item;
	int32 Index = 0;
	FSimpleDelegate OnChanged;
};
