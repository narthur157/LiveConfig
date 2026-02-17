// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#pragma once

#include "Styling/SlateStyle.h"

class FLiveConfigPropertyStyle
{
public:
	static void Initialize();
	static void Shutdown();
	static ISlateStyle& Get() { return *StyleSet.Get(); }
	static FName GetStyleSetName();

	static const FSlateBrush* GetBrush(FName PropertyName, const ANSICHAR* Specifier = nullptr)
	{
		return StyleSet->GetBrush(PropertyName, Specifier);
	}

	static const FLinearColor& GetColor(FName PropertyName, const ANSICHAR* Specifier = nullptr)
	{
		return StyleSet->GetColor(PropertyName, Specifier);
	}

private:
	static TSharedPtr<FSlateStyleSet> StyleSet;
};
