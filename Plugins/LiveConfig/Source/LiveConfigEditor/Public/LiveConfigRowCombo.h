// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LiveConfigRowNamePin.h"
#include "Widgets/SCompoundWidget.h"

/**
 * 
 */
class SLiveConfigRowCombo : public SCompoundWidget
{
	SLATE_DECLARE_WIDGET_API(SLiveConfigRowCombo, SCompoundWidget, LIVECONFIGEDITOR_API)
	
public:
	SLiveConfigRowCombo();
    DECLARE_DELEGATE_OneParam(FOnRowNameChanged, FLiveConfigRowName);
	
	SLATE_BEGIN_ARGS(SLiveConfigRowCombo) { }
		// Flag to set if the list is read only
		SLATE_ARGUMENT(bool, ReadOnly)
		
		SLATE_ATTRIBUTE(FLiveConfigRowName, RowName)
		
		SLATE_EVENT(FOnRowNameChanged, OnRowNameChanged)
		
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
protected:
	FOnRowNameChanged OnRowNameChanged;
	TSlateAttribute<FLiveConfigRowName> RowNameAttribute;

	void OnRowNameSelected(FLiveConfigRowName RowName);
};
