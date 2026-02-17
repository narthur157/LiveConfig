#include "SLiveConfigRedirectManager.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "LiveConfigSystem.h"
#include "Misc/MessageDialog.h"
#include "Editor.h"
#include "LiveConfigEditorLib.h"

#define LOCTEXT_NAMESPACE "SLiveConfigRedirectManager"

SLATE_IMPLEMENT_WIDGET(SLiveConfigRedirectManager);

void SLiveConfigRedirectManager::PrivateRegisterAttributes(FSlateAttributeInitializer& AttributeInitializer)
{
}

void SLiveConfigRedirectManager::Construct(const FArguments& InArgs)
{
	ULiveConfigSystem& System = ULiveConfigSystem::Get();

	// Get all redirects and check which are unused
	TArray<FName> UnusedRedirects;
	ULiveConfigEditorLib::GetUnusedRedirects(UnusedRedirects);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.Padding(10)
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(FText::Format(
				LOCTEXT("RedirectsInfo", "Total Redirects: {0} | Unused Redirects: {1}"),
				FText::AsNumber(System.PropertyRedirects.Num()),
				FText::AsNumber(UnusedRedirects.Num())
			))
			.Font(FAppStyle::GetFontStyle("BoldFont"))
		]
		+ SVerticalBox::Slot()
		.Padding(10, 5, 10, 0)
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("RedirectsInfoHelp", "Redirects allow renamed properties to continue working with old references.\nCheck blueprints/code before removing. Unused redirects point to deleted properties."))
			.AutoWrapText(true)
			.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f))
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(10, 10, 10, 10)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(5)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SAssignNew(RedirectListBox, SVerticalBox)
				]
			]
		]
		+ SVerticalBox::Slot()
		.Padding(10, 0, 10, 10)
		.AutoHeight()
		.HAlign(HAlign_Right)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 5, 0)
			[
				SNew(SButton)
				.Text(LOCTEXT("CleanupUnusedRedirects", "Cleanup Unused"))
				.IsEnabled(UnusedRedirects.Num() > 0)
				.OnClicked_Lambda([this]()
				{
					OnCleanupUnusedRedirects();
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("Close", "Close"))
				.OnClicked_Lambda([this]()
				{
					// Find and close the parent window
					TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
					if (ParentWindow.IsValid())
					{
						ParentWindow->RequestDestroyWindow();
					}
					return FReply::Handled();
				})
			]
		]
	];

	RefreshRedirectList();
}

void SLiveConfigRedirectManager::RefreshRedirectList()
{
	if (!RedirectListBox.IsValid())
	{
		return;
	}

	RedirectListBox->ClearChildren();

	ULiveConfigSystem& System = ULiveConfigSystem::Get();
	TArray<FName> UnusedRedirects;
	ULiveConfigEditorLib::GetUnusedRedirects(UnusedRedirects);

	if (System.PropertyRedirects.Num() == 0)
	{
		RedirectListBox->AddSlot()
		.Padding(5)
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoRedirects", "No redirects found."))
			.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f))
		];
	}
	else
	{
		// Sort redirects alphabetically
		TArray<TPair<FName, FName>> SortedRedirects;
		for (const auto& Pair : System.PropertyRedirects)
		{
			SortedRedirects.Add(TPair<FName, FName>(Pair.Key, Pair.Value));
		}
		SortedRedirects.Sort([](const TPair<FName, FName>& A, const TPair<FName, FName>& B)
		{
			return A.Key.LexicalLess(B.Key);
		});

		for (const auto& Pair : SortedRedirects)
		{
			const FName& OldName = Pair.Key;
			const FName& NewName = Pair.Value;
			const bool bIsUnused = UnusedRedirects.Contains(OldName);

			RedirectListBox->AddSlot()
			.Padding(2)
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(bIsUnused ? FLinearColor(0.3f, 0.1f, 0.1f, 0.5f) : FLinearColor(0.1f, 0.1f, 0.1f, 0.3f))
				.Padding(8)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(0, 0, 10, 0)
						[
							SNew(STextBlock)
							.Text(FText::FromName(OldName))
							.Font(FAppStyle::GetFontStyle("NormalFont"))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(0, 0, 10, 0)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("RedirectArrow", "→"))
							.Font(FAppStyle::GetFontStyle("BoldFont"))
							.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromName(NewName))
							.Font(FAppStyle::GetFontStyle("NormalFont"))
							.ColorAndOpacity(bIsUnused ? FLinearColor(0.8f, 0.3f, 0.3f, 1.0f) : FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(10, 0, 0, 0)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(bIsUnused ? LOCTEXT("UnusedLabel", "(Target Missing)") : FText::GetEmpty())
							.Font(FAppStyle::GetFontStyle("SmallFont"))
							.ColorAndOpacity(FLinearColor(0.8f, 0.3f, 0.3f, 1.0f))
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(5, 0, 0, 0)
					.VAlign(VAlign_Center)
					[
						SNew(SButton)
						.ButtonStyle(FAppStyle::Get(), "SimpleButton")
						.Text(LOCTEXT("CheckUsage", "Check Usage"))
						.ToolTipText(LOCTEXT("CheckUsageTooltip", "Show all assets referencing this old property name in the Reference Viewer.\nNote: Does not check C++ code - search project manually for code references."))
						.OnClicked_Lambda([this, OldName]()
						{
							OnCheckUsage(OldName);
							return FReply::Handled();
						})
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(5, 0, 0, 0)
					.VAlign(VAlign_Center)
					[
						SNew(SButton)
						.ButtonStyle(FAppStyle::Get(), "SimpleButton")
						.ContentPadding(FMargin(4, 2))
						.OnClicked_Lambda([this, OldName]()
						{
							OnRemoveRedirect(OldName);
							return FReply::Handled();
						})
						.ToolTipText(LOCTEXT("RemoveRedirectTooltip", "Remove this redirect"))
						[
							SNew(SImage)
							.Image(FAppStyle::GetBrush("Icons.Delete"))
						]
					]
				]
			];
		}
	}
}

void SLiveConfigRedirectManager::OnCheckUsage(FName OldPropertyName)
{
	TArray<FName> RelatedNames;
	ULiveConfigEditorLib::GetRelatedPropertyNames(OldPropertyName, RelatedNames);

	TArray<FAssetIdentifier> AssetIdentifiers;
	for (const FName& Name : RelatedNames)
	{
		AssetIdentifiers.Add(FAssetIdentifier(FLiveConfigProperty::StaticStruct(), Name));
	}

	FEditorDelegates::OnOpenReferenceViewer.Broadcast(AssetIdentifiers, FReferenceViewerParams());
}

void SLiveConfigRedirectManager::OnRemoveRedirect(FName OldPropertyName)
{
	ULiveConfigSystem::Get().RemoveRedirect(OldPropertyName);
	RefreshRedirectList();
}

void SLiveConfigRedirectManager::OnCleanupUnusedRedirects()
{
	int32 NumRemoved = ULiveConfigEditorLib::CleanupUnusedRedirects();

	FText Message = FText::Format(
		LOCTEXT("RedirectsCleanedUp", "Removed {0} unused redirect(s)"),
		FText::AsNumber(NumRemoved)
	);
	FMessageDialog::Open(EAppMsgType::Ok, Message);

	RefreshRedirectList();
}

#undef LOCTEXT_NAMESPACE
