// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#include "SLiveConfigCleanupUnusedPropertiesWidget.h"

#include "LiveConfigEditorLib.h"
#include "LiveConfigSystem.h"
#include "LiveConfigJson.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Misc/MessageDialog.h"
#include "Misc/ScopedSlowTask.h"
#include "AssetRegistry/AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "LiveConfigCleanup"

class SUnusedPropertyListRow : public SMultiColumnTableRow<TSharedPtr<FUnusedLiveConfigPropertyItem>>
{
public:
	SLATE_BEGIN_ARGS(SUnusedPropertyListRow) {}
		SLATE_ARGUMENT(TSharedPtr<FUnusedLiveConfigPropertyItem>, Item)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		Item = InArgs._Item;
		SMultiColumnTableRow<TSharedPtr<FUnusedLiveConfigPropertyItem>>::Construct(FSuperRowType::FArguments(), InOwnerTableView);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (ColumnName == "Check")
		{
			return SNew(SBox)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([this]() { return Item->CheckState; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { Item->CheckState = NewState; })
				];
		}
		else if (ColumnName == "Name")
		{
			return SNew(STextBlock)
				.Text(FText::FromName(Item->PropertyName))
				.Margin(FMargin(4, 2));
		}

		return SNullWidget::NullWidget;
	}

private:
	TSharedPtr<FUnusedLiveConfigPropertyItem> Item;
};

void SLiveConfigCleanupUnusedPropertiesWidget::Construct(const FArguments& InArgs)
{
	PopulateUnusedProperties();

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8)
		[
			SNew(STextBlock)
			.Text(this, &SLiveConfigCleanupUnusedPropertiesWidget::GetDescriptionText)
			.AutoWrapText(true)
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(8, 0)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SAssignNew(UnusedPropertiesListView, SListView<TSharedPtr<FUnusedLiveConfigPropertyItem>>)
				.ListItemsSource(&UnusedProperties)
				.OnGenerateRow(this, &SLiveConfigCleanupUnusedPropertiesWidget::MakeUnusedPropertyListItemWidget)
				.HeaderRow(
					SNew(SHeaderRow)
					+ SHeaderRow::Column("Check")
					.DefaultLabel(FText::GetEmpty())
					.FixedWidth(24)
					[
						SNew(SCheckBox)
						.IsChecked(this, &SLiveConfigCleanupUnusedPropertiesWidget::GetToggleSelectedState)
						.OnCheckStateChanged(this, &SLiveConfigCleanupUnusedPropertiesWidget::OnToggleSelectedCheckBox)
					]
					+ SHeaderRow::Column("Name")
					.DefaultLabel(LOCTEXT("PropertyNameColumn", "Property Name"))
					.FillWidth(1.0f)
				)
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Right)
		.Padding(8)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("RemoveSelected", "Remove Selected"))
				.OnClicked(this, &SLiveConfigCleanupUnusedPropertiesWidget::OnRemovePressed)
				.IsEnabled(this, &SLiveConfigCleanupUnusedPropertiesWidget::IsRemoveEnabled)
			]
		]
	];
}

TSharedRef<ITableRow> SLiveConfigCleanupUnusedPropertiesWidget::MakeUnusedPropertyListItemWidget(TSharedPtr<FUnusedLiveConfigPropertyItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SUnusedPropertyListRow, OwnerTable).Item(Item);
}

void SLiveConfigCleanupUnusedPropertiesWidget::PopulateUnusedProperties()
{
	UnusedProperties.Empty();

	TArray<FName> UnusedNames;
	ULiveConfigEditorLib::GetUnusedProperties(UnusedNames);

	for (const FName& Name : UnusedNames)
	{
		UnusedProperties.Add(MakeShared<FUnusedLiveConfigPropertyItem>(Name));
	}

	if (UnusedPropertiesListView.IsValid())
	{
		UnusedPropertiesListView->RequestListRefresh();
	}
}

FReply SLiveConfigCleanupUnusedPropertiesWidget::OnRemovePressed()
{
	ULiveConfigSystem& System = ULiveConfigSystem::Get();
	ULiveConfigJsonSystem* JsonSystem = ULiveConfigJsonSystem::Get();

	TArray<FName> NamesToDelete;
	for (const auto& Item : UnusedProperties)
	{
		if (Item->CheckState == ECheckBoxState::Checked)
		{
			NamesToDelete.Add(Item->PropertyName);
		}
	}

	if (NamesToDelete.Num() > 0)
	{
		FScopedSlowTask SlowTask((float)NamesToDelete.Num(), LOCTEXT("RemovingProperties", "Removing Properties"));
		SlowTask.MakeDialog();

		for (const FName& Name : NamesToDelete)
		{
			SlowTask.EnterProgressFrame();
			System.PropertyDefinitions.Remove(Name);
			if (JsonSystem)
			{
				JsonSystem->DeletePropertyFile(Name);
			}
		}

		System.RebuildConfigCache();
		PopulateUnusedProperties();

		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("PropertiesRemoved", "Successfully removed {0} property(s)."), FText::AsNumber(NamesToDelete.Num())));
	}

	return FReply::Handled();
}

bool SLiveConfigCleanupUnusedPropertiesWidget::IsRemoveEnabled() const
{
	for (const auto& Item : UnusedProperties)
	{
		if (Item->CheckState == ECheckBoxState::Checked)
		{
			return true;
		}
	}
	return false;
}

ECheckBoxState SLiveConfigCleanupUnusedPropertiesWidget::GetToggleSelectedState() const
{
	int32 NumChecked = 0;
	for (const auto& Item : UnusedProperties)
	{
		if (Item->CheckState == ECheckBoxState::Checked)
		{
			NumChecked++;
		}
	}

	if (NumChecked == 0) return ECheckBoxState::Unchecked;
	if (NumChecked == UnusedProperties.Num()) return ECheckBoxState::Checked;
	return ECheckBoxState::Undetermined;
}

void SLiveConfigCleanupUnusedPropertiesWidget::OnToggleSelectedCheckBox(ECheckBoxState InNewState)
{
	for (auto& Item : UnusedProperties)
	{
		Item->CheckState = InNewState;
	}
	if (UnusedPropertiesListView.IsValid())
	{
		UnusedPropertiesListView->RequestListRefresh();
	}
}

FText SLiveConfigCleanupUnusedPropertiesWidget::GetDescriptionText() const
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	if (AssetRegistryModule.Get().IsLoadingAssets())
	{
		return LOCTEXT("CleanupDescription_Loading", "Still discovering assets. Please wait...");
	}

	if (UnusedProperties.Num() == 0)
	{
		return LOCTEXT("CleanupDescription_None", "All Live Config properties are currently referenced by assets or config files.");
	}

	return FText::Format(LOCTEXT("CleanupDescription_Found", "The following {0} Live Config properties are not referenced by any assets or config files. They are safe to remove if you don't use them in C++ code."), FText::AsNumber(UnusedProperties.Num()));
}

#undef LOCTEXT_NAMESPACE
