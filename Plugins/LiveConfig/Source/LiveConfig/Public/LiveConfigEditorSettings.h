// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LiveConfigSystem.h"
#include "LiveConfigEditorSettings.generated.h"

UENUM()
enum class ELiveConfigRedirectMode : uint8
{
	/** Always create redirects automatically when renaming properties */
	AlwaysCreate,
	/** Never create redirects when renaming properties */
	NeverCreate,
	/** Prompt the user to choose whether to create redirects */
	Prompt
};

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

	/**
	 * Controls how redirects are created when renaming properties in the Property Manager
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Property Manager")
	ELiveConfigRedirectMode RedirectMode = ELiveConfigRedirectMode::AlwaysCreate;
};

