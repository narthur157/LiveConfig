// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

DECLARE_DELEGATE_OneParam(FOnTagSelected, FName);

class SLiveConfigTagPicker : public SCompoundWidget
{
	SLATE_DECLARE_WIDGET(SLiveConfigTagPicker, SCompoundWidget);
public:
	SLiveConfigTagPicker();

	SLATE_BEGIN_ARGS(SLiveConfigTagPicker)
		: _TagVisibilityFilter()
	{}
		SLATE_EVENT(FOnTagSelected, OnTagSelected)
		SLATE_EVENT(FOnTagSelected, OnAddNewTag)
		SLATE_ATTRIBUTE(TArray<FName>, TagOptions)
		SLATE_ARGUMENT(TFunction<bool(FName)>, TagVisibilityFilter)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedRef<SWidget> GenerateMenuContent();
	void ShowNewTagDialog();

	FOnTagSelected OnTagSelected;
	FOnTagSelected OnAddNewTag;
	TSlateAttribute<TArray<FName>> TagOptionsAttribute;
	TFunction<bool(FName)> TagVisibilityFilter;
};
