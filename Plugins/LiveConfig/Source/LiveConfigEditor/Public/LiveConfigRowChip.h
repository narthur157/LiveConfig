// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * 
 */
class SLiveConfigRowChip : public SCompoundWidget
{
	SLATE_DECLARE_WIDGET_API(SLiveConfigRowChip, SCompoundWidget, LIVECONFIGEDITOR_API)
public:
	SLiveConfigRowChip();
	
	DECLARE_DELEGATE_RetVal(FReply, FOnEditPressed);
	DECLARE_DELEGATE_RetVal(FReply, FOnClearPressed);
	
	SLATE_BEGIN_ARGS(SLiveConfigRowChip) {}
		SLATE_ARGUMENT(bool, ReadOnly)
		SLATE_ATTRIBUTE(FText, TooltipText)
		SLATE_ATTRIBUTE(FText, Text)
		SLATE_ATTRIBUTE(bool, IsSelected)
		// If true, shows clear button inside the chip
		SLATE_ATTRIBUTE(bool, ShowClearButton)
		SLATE_EVENT(FOnEditPressed, OnEditPressed)
		SLATE_EVENT(FOnClearPressed, OnClearPressed)
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	TSharedPtr<SButton> ChipButton;
	TSharedPtr<SButton> ClearButton;

	TSlateAttribute<FText> ToolTipTextAttribute;
	TSlateAttribute<FText> TextAttribute;
	TSlateAttribute<bool> IsSelectedAttribute;
	TSlateAttribute<bool> ShowClearButtonAttribute;
	bool bLastHasIsSelected = false;
	
	FOnEditPressed OnEditPressed;
	FOnClearPressed OnClearPressed;
protected:
	void UpdatePillStyle();
};
