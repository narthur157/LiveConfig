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
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite)
	bool bPollConfigInEditor = true;

	/**
	 * How often to refresh the config in editor
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite)
	float EditorPollRateMinutes = 30;
	
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite)
	TMap<FLiveConfigRowName, FLiveConfigPropertyDefinition> PropertyDefinitions;
};

