// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LiveConfigSystem.h"
#include "LiveConfigEditorSettings.generated.h"

/**
 * 
 */
UCLASS(Config=Editor, DefaultConfig)
class LIVECONFIG_API ULiveConfigEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	virtual FName GetCategoryName() const override { return TEXT("Project"); }
	virtual FName GetSectionName() const override { return TEXT("Live Config Editor"); }

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "GoogleSheets")
	bool bPollConfigInEditor = true;

	/**
	 * How often to refresh the config in editor
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "GoogleSheets")
	float EditorPollRateMinutes = 30;
};

