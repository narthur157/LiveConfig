// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "LiveConfigTypes.h"

#include "StructViewerFilter.h"

class FLiveConfigStructFilter : public IStructViewerFilter
{
public:
	FLiveConfigStructFilter();

	virtual bool IsStructAllowed(const FStructViewerInitializationOptions& InInitOptions, const UScriptStruct* InStruct, TSharedRef<class FStructViewerFilterFuncs> InNode) override;

	virtual bool IsUnloadedStructAllowed(const FStructViewerInitializationOptions& InInitOptions, const FSoftObjectPath& InStructPath, TSharedRef<class FStructViewerFilterFuncs> InNode) override;

private:
	bool IsPackageAllowed(const FString& PackageName) const;

	TArray<FString> AllowedScripts;
};

class LIVECONFIGEDITOR_API SLiveConfigPropertyValueWidget : public SCompoundWidget
{
public:
	SLATE_DECLARE_WIDGET(SLiveConfigPropertyValueWidget, SCompoundWidget);

	DECLARE_DELEGATE_OneParam(FOnValueChanged, const FString&);

	SLATE_BEGIN_ARGS(SLiveConfigPropertyValueWidget)
		: _Value()
		, _PropertyType(ELiveConfigPropertyType::String)
		, _bReadOnly(false)
	{}
		SLATE_ATTRIBUTE(FString, Value);
		SLATE_ATTRIBUTE(ELiveConfigPropertyType, PropertyType);
		SLATE_ATTRIBUTE(bool, bReadOnly);
		SLATE_EVENT(FOnValueChanged, OnValueChanged);
		SLATE_EVENT(FSimpleDelegate, OnEnter);
	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs);

	void RequestFocus();

private:
	TSharedRef<SWidget> OnGetStructPickerMenu();
	void OnStructPicked(const UScriptStruct* ChosenStruct);
	bool VerifyValueText(const FText& NewText, FText& OutError);
	void ValueTextCommitted(const FText& NewText, ETextCommit::Type CommitType);

	TAttribute<FString> ValueAttribute;
	TAttribute<ELiveConfigPropertyType> PropertyTypeAttribute;
	TAttribute<bool> bReadOnlyAttribute = false;
	FOnValueChanged OnValueChanged;
	FSimpleDelegate OnEnter;

	TSharedPtr<class SEditableTextBox> ValueTextBox;
	TSharedPtr<class SCheckBox> ValueCheckBox;
	TSharedPtr<class SComboButton> StructComboButton;
};
