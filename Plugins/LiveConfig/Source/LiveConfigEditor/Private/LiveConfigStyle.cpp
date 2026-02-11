#include "LiveConfigStyle.h"

#include "Styling/SlateStyleMacros.h"
#include "Styling/SlateStyleRegistry.h"

TSharedPtr<FLiveConfigStyle> FLiveConfigStyle::StyleInstance = nullptr;

void FLiveConfigStyle::Initialize()
{
	FSlateStyleRegistry::RegisterSlateStyle(Get());
}

void FLiveConfigStyle::Shutdown()
{
	if (StyleInstance.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(Get());
		StyleInstance.Reset();
	}
}

const ISlateStyle& FLiveConfigStyle::Get()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = MakeShareable(new FLiveConfigStyle());
	}
	
	return *StyleInstance;
}

FLiveConfigStyle::FLiveConfigStyle() : FSlateStyleSet("LiveConfigStyle")
{
	const FVector2f Icon8x8(8.0f, 8.0f);
	const FVector2f Icon16x16(16.0f, 16.0f);
	const FVector2f Icon20x20(20.0f, 20.0f);
	const FVector2f Icon40x40(40.0f, 40.0f);
	const FVector2f Icon48x48(48.0f, 48.0f);

	FSlateStyleSet::SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate"));
	FSlateStyleSet::SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

	const FSlateColor SelectionColor = FLinearColor(1, 1, 1, .2f);
	const FSlateColor SelectionColor_Inactive = FLinearColor(1, 1, 1, .10f);
	const FSlateColor SelectorColor = FLinearColor(1, 1, 1);
	const FSlateColor DefaultTextColor = FLinearColor(1, 1, 1);
	const FSlateColor TextSelectedColor = FLinearColor(1, 1, 1);
	const FSlateColor EvenColor = FLinearColor(1, 1, 1, .08);
	const FSlateColor OddColor = FLinearColor(1,1,1, .1);
	
	FTableRowStyle NormalTableRowStyle = FTableRowStyle()
	.SetEvenRowBackgroundBrush(FSlateNoResource())
	.SetEvenRowBackgroundHoveredBrush(IMAGE_BRUSH("Common/Selection", Icon8x8, FLinearColor(1.0f, 1.0f, 1.0f, 1.f)))
	.SetOddRowBackgroundBrush(FSlateNoResource())
	.SetOddRowBackgroundHoveredBrush(IMAGE_BRUSH("Common/Selection", Icon8x8, FLinearColor(1.0f, 1.0f, 1.0f, 1.f)))
	.SetSelectorFocusedBrush(BORDER_BRUSH("Common/Selector", FMargin(4.f / 16.f), SelectorColor))
	.SetActiveBrush(IMAGE_BRUSH("Common/Selection", Icon8x8, SelectionColor))
	.SetActiveHoveredBrush(IMAGE_BRUSH("Common/Selection", Icon8x8, SelectionColor))
	.SetInactiveBrush(IMAGE_BRUSH("Common/Selection", Icon8x8, SelectionColor_Inactive))
	.SetInactiveHoveredBrush(IMAGE_BRUSH("Common/Selection", Icon8x8, SelectionColor_Inactive))
	.SetTextColor(DefaultTextColor)
	.SetSelectedTextColor(TextSelectedColor)
	.SetDropIndicator_Above(BOX_BRUSH("Common/DropZoneIndicator_Above", FMargin(10.0f / 16.0f, 10.0f / 16.0f, 0, 0), SelectionColor))
	.SetDropIndicator_Onto(BOX_BRUSH("Common/DropZoneIndicator_Onto", FMargin(4.0f / 16.0f), SelectionColor))
	.SetDropIndicator_Below(BOX_BRUSH("Common/DropZoneIndicator_Below", FMargin(10.0f / 16.0f, 0, 0, 10.0f / 16.0f), SelectionColor));

	Set("LiveConfig.TableRow", FTableRowStyle(NormalTableRowStyle)
		.SetEvenRowBackgroundBrush(IMAGE_BRUSH("Common/Selection", Icon8x8, EvenColor))
		.SetEvenRowBackgroundHoveredBrush(IMAGE_BRUSH("PropertyView/DetailCategoryMiddle_Hovered", Icon16x16))
		.SetOddRowBackgroundBrush(IMAGE_BRUSH("Common/Selection", Icon8x8, OddColor))
		.SetOddRowBackgroundHoveredBrush(IMAGE_BRUSH("PropertyView/DetailCategoryMiddle_Hovered", Icon16x16))
		.SetSelectorFocusedBrush(BORDER_BRUSH("Common/Selector", FMargin(4.f / 16.f), SelectorColor))
		.SetActiveBrush(IMAGE_BRUSH("Common/Selection", Icon8x8, SelectionColor))
		.SetActiveHoveredBrush(IMAGE_BRUSH("Common/Selection", Icon8x8, SelectionColor))
		.SetInactiveBrush(IMAGE_BRUSH("Common/Selection", Icon8x8, SelectionColor_Inactive))
		.SetInactiveHoveredBrush(IMAGE_BRUSH("Common/Selection", Icon8x8, SelectionColor_Inactive))
		.SetTextColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.f))
		);
}
