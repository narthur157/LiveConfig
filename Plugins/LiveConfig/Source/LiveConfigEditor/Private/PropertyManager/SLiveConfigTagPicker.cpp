#include "SLiveConfigTagPicker.h"

#include "LiveConfigGameSettings.h"
#include "LiveConfigSystem.h"
#include "SLiveConfigNewTagDialog.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "LiveConfigTagPicker"

SLATE_IMPLEMENT_WIDGET(SLiveConfigTagPicker);

SLiveConfigTagPicker::SLiveConfigTagPicker()
	: TagOptionsAttribute(*this)
{
}

void SLiveConfigTagPicker::PrivateRegisterAttributes(FSlateAttributeInitializer& AttributeInitializer)
{
	SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "TagOptions", TagOptionsAttribute, EInvalidateWidgetReason::Layout);
}

void SLiveConfigTagPicker::Construct(const FArguments& InArgs)
{
	OnTagSelected = InArgs._OnTagSelected;
	OnAddNewTag = InArgs._OnAddNewTag;
	TagOptionsAttribute.Assign(*this, InArgs._TagOptions);
	TagVisibilityFilter = InArgs._TagVisibilityFilter;

	ChildSlot
	[
		GenerateMenuContent()
	];
}

TSharedRef<SWidget> SLiveConfigTagPicker::GenerateMenuContent()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	TArray<FName> AvailableTags = TagOptionsAttribute.IsBound(*this) ? TagOptionsAttribute.Get() : ULiveConfigSystem::Get().PropertyTags;
	
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
				}))
			);
			bAnyTagsVisible = true;
		}
	}

	if (!bAnyTagsVisible)
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("NoTagsAvailable", "No tags available"),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction(),
			NAME_None,
			EUserInterfaceActionType::None
		);
	}

	MenuBuilder.AddMenuSeparator();
	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddNewTagMenu", "Add New Tag..."),
		FText::GetEmpty(),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"),
		FUIAction(FExecuteAction::CreateLambda([this]()
		{
			ShowNewTagDialog();
		}))
	);

	return MenuBuilder.MakeWidget();
}

void SLiveConfigTagPicker::ShowNewTagDialog()
{
	SLiveConfigNewTagDialog::OpenDialog(FOnTagCreated::CreateLambda([this](FName NewTag)
	{
		OnAddNewTag.ExecuteIfBound(NewTag);
		OnTagSelected.ExecuteIfBound(NewTag);
	}));
}

#undef LOCTEXT_NAMESPACE
