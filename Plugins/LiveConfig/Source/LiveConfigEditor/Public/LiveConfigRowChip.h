// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * 
 */
class LIVECONFIGEDITOR_API SLiveConfigRowChip : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLiveConfigRowChip)
		{
		}

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
};
