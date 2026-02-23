// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#pragma once

#include "CoreMinimal.h"
#include "SLiveConfigPropertyPin.h"
#include "Widgets/SCompoundWidget.h"

class SLiveConfigPropertyPicker;
class SLiveConfigPropertyChip;
/**
 * This is the button you can press to summon the property picker
 * Supports filtering based on property type
 */
class SLiveConfigPropertyCombo : public SCompoundWidget
{
	SLATE_DECLARE_WIDGET_API(SLiveConfigPropertyCombo, SCompoundWidget, LIVECONFIGEDITOR_API)
	
public:
	SLiveConfigPropertyCombo();
    DECLARE_DELEGATE_OneParam(FOnPropertyChanged, FLiveConfigProperty)
	
	SLATE_BEGIN_ARGS(SLiveConfigPropertyCombo) { }
		SLATE_ARGUMENT(bool, ReadOnly)
		
		SLATE_ATTRIBUTE(FLiveConfigProperty, Property)
		SLATE_ATTRIBUTE(TOptional<ELiveConfigPropertyType>, FilterType)
		SLATE_ATTRIBUTE(UScriptStruct*, StructFilter)
		
		SLATE_EVENT(FOnPropertyChanged, OnPropertyChanged)
	
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
protected:
	bool ShowClearButton() const;
	void OnPropertySelected(FLiveConfigProperty Property);

	TSharedRef<SWidget> OnGetMenuContent();
	void OnMenuOpenChanged(bool bOpen);
	
	TSharedPtr<SWidget> OnGetContextMenuContent();
	FReply OnEditPressed();
	void OnCopyProperty();
	void OnPasteProperty();
	bool CanPasteProperty() const;
	void OnClearProperty();
	FText GetChipText() const;
	
	FOnPropertyChanged OnPropertyChanged;
	TSlateAttribute<FLiveConfigProperty> RowNameAttribute;
	TSlateAttribute<TOptional<ELiveConfigPropertyType>> FilterTypeAttribute;
	TSlateAttribute<UScriptStruct*> StructFilterAttribute;
	TSharedPtr<SComboButton> ComboButton;
	TSharedPtr<SLiveConfigPropertyPicker> PropertyPicker;
	TSharedPtr<SLiveConfigPropertyChip> Chip;

};
