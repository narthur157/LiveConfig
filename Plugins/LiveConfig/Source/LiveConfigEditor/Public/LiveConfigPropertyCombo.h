// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LiveConfigPropertyPin.h"
#include "Widgets/SCompoundWidget.h"

class SLiveConfigPropertyChip;
/**
 * 
 */
class SLiveConfigPropertyCombo : public SCompoundWidget
{
	SLATE_DECLARE_WIDGET_API(SLiveConfigPropertyCombo, SCompoundWidget, LIVECONFIGEDITOR_API)
	
public:
	SLiveConfigPropertyCombo();
    DECLARE_DELEGATE_OneParam(FOnPropertyChanged, FLiveConfigProperty);
	
	SLATE_BEGIN_ARGS(SLiveConfigPropertyCombo) { }
		// Flag to set if the list is read only
		SLATE_ARGUMENT(bool, ReadOnly)
		
		SLATE_ATTRIBUTE(FLiveConfigProperty, Property)
		SLATE_ATTRIBUTE(TOptional<ELiveConfigPropertyType>, FilterType)
		
		SLATE_EVENT(FOnPropertyChanged, OnPropertyChanged)
		
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
protected:
	bool ShowClearButton() const;
	void OnPropertySelected(FLiveConfigProperty Property);

	TSharedPtr<SWidget> OnGetContextMenuContent();
	void OnCopyProperty();
	void OnPasteProperty();
	bool CanPasteProperty() const;
	void OnClearProperty();
	
	FOnPropertyChanged OnPropertyChanged;
	TSlateAttribute<FLiveConfigProperty> RowNameAttribute;
	TSlateAttribute<TOptional<ELiveConfigPropertyType>> FilterTypeAttribute;
	TSharedPtr<SComboButton> ComboButton;
	TSharedPtr<SLiveConfigPropertyChip> Chip;

};
