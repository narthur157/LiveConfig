#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

DECLARE_DELEGATE_OneParam(FOnTagSelected, FName);

class SLiveConfigTagPicker : public SCompoundWidget
{
	SLATE_DECLARE_WIDGET(SLiveConfigTagPicker, SCompoundWidget);
public:
	SLiveConfigTagPicker() : KnownTagsAttribute(*this) {}

	SLATE_BEGIN_ARGS(SLiveConfigTagPicker)
		: _TagVisibilityFilter()
	{}
		SLATE_EVENT(FOnTagSelected, OnTagSelected)
		SLATE_EVENT(FSimpleDelegate, OnAddNewTag)
		SLATE_ATTRIBUTE(TArray<FName>, KnownTags)
		SLATE_ARGUMENT(TFunction<bool(FName)>, TagVisibilityFilter)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedRef<SWidget> GenerateMenuContent();

	FOnTagSelected OnTagSelected;
	FSimpleDelegate OnAddNewTag;
	TSlateAttribute<TArray<FName>> KnownTagsAttribute;
	TFunction<bool(FName)> TagVisibilityFilter;
};
