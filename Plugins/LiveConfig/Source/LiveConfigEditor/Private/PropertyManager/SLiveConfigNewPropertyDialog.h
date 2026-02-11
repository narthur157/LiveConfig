#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "LiveConfigTypes.h"
#include "LiveConfigPropertyName.h"

DECLARE_DELEGATE_OneParam(FOnPropertyCreated, const FLiveConfigPropertyDefinition&);

class SLiveConfigNewPropertyDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLiveConfigNewPropertyDialog)
		: _InitialName()
		, _InitialType(ELiveConfigPropertyType::String)
		, _InitialTags()
	{}
		SLATE_ARGUMENT(FString, InitialName);
		SLATE_ARGUMENT(ELiveConfigPropertyType, InitialType);
		SLATE_ARGUMENT(TArray<FName>, InitialTags);
		SLATE_EVENT(FOnPropertyCreated, OnPropertyCreated);
	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

	static void OpenDialog(const FString& InInitialName, ELiveConfigPropertyType InInitialType, const TArray<FName>& InInitialTags, FOnPropertyCreated InOnPropertyCreated);
	
private:
	FReply HandleCreate();
	FReply OnCancelClicked();
	
	void OnTypeSelected(TSharedPtr<ELiveConfigPropertyType> NewType, ESelectInfo::Type SelectInfo);
	TSharedRef<SWidget> OnGenerateTypeWidget(TSharedPtr<ELiveConfigPropertyType> InType);
	FText GetSelectedTypeText() const;

	void OnTagSelected(FName TagName);
	void RefreshTags();

	TSharedPtr<class SWrapBox> TagWrapBox;
	TSharedPtr<class SEditableTextBox> NameTextBox;
	TSharedPtr<class SEditableTextBox> DescriptionTextBox;
	TSharedPtr<class SComboBox<TSharedPtr<ELiveConfigPropertyType>>> TypeComboBox;
	TArray<TSharedPtr<ELiveConfigPropertyType>> TypeOptions;
	
	ELiveConfigPropertyType SelectedType = ELiveConfigPropertyType::String;
	TSharedPtr<FLiveConfigPropertyDefinition> TempDefinition;

	FOnPropertyCreated OnPropertyCreated;
	TWeakPtr<SWindow> WindowPtr;
};
