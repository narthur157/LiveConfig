#include "SLiveConfigTagPicker.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "LiveConfigTagPicker"

SLATE_IMPLEMENT_WIDGET(SLiveConfigTagPicker);
void SLiveConfigTagPicker::PrivateRegisterAttributes(FSlateAttributeInitializer& AttributeInitializer)
{
	SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "KnownTags", KnownTagsAttribute, EInvalidateWidgetReason::Layout);
}

void SLiveConfigTagPicker::Construct(const FArguments& InArgs)
{
	OnTagSelected = InArgs._OnTagSelected;
	OnAddNewTag = InArgs._OnAddNewTag;
	KnownTagsAttribute.Assign(*this, InArgs._KnownTags);
	TagVisibilityFilter = InArgs._TagVisibilityFilter;

	ChildSlot
	[
		GenerateMenuContent()
	];
}

TSharedRef<SWidget> SLiveConfigTagPicker::GenerateMenuContent()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	TArray<FName> AvailableTags = KnownTagsAttribute.Get();
	AvailableTags.Sort([](const FName& A, const FName& B) { return A.Compare(B) < 0; });

	bool bAnyTagsVisible = false;
	for (const FName& AvailableTag : AvailableTags)
	{
		if (!TagVisibilityFilter || TagVisibilityFilter(AvailableTag))
		{
			MenuBuilder.AddMenuEntry(
				FText::FromName(AvailableTag),
				FText::Format(LOCTEXT("AddTagTooltip", "Add tag '{0}'"), FText::FromName(AvailableTag)),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([this, AvailableTag]()
				{
					OnTagSelected.ExecuteIfBound(AvailableTag);
					// Note: Since this widget is typically hosted in a SComboButton or Menu, 
					// the menu will close automatically upon selection.
				}))
			);
			bAnyTagsVisible = true;
		}
	}

	if (!bAnyTagsVisible)
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("NoTagsAvailable", "No more tags available"),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction(),
			NAME_None,
			EUserInterfaceActionType::None
		);
	}

	if (OnAddNewTag.IsBound())
	{
		MenuBuilder.AddMenuSeparator();
		MenuBuilder.AddMenuEntry(
			LOCTEXT("AddNewTagMenu", "Add New Tag..."),
			FText::GetEmpty(),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"),
			FUIAction(FExecuteAction::CreateLambda([this]()
			{
				OnAddNewTag.ExecuteIfBound();
			}))
		);
	}

	return MenuBuilder.MakeWidget();
}

#undef LOCTEXT_NAMESPACE
