// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#pragma once

class FSlateStyleSet;

#include "Styling/SlateStyle.h"

class FLiveConfigStyle : public FSlateStyleSet
{
public:

	static void Initialize();
	static void Shutdown();
	static const ISlateStyle& Get();
private:
	FLiveConfigStyle();
	static TSharedPtr<FLiveConfigStyle> StyleInstance;
};
