// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "LiveConfigTypes.h"
#include "LiveConfigEditorSubsystem.generated.h"

class ULiveConfigUserSettings;
class SNotificationItem;
struct FFileChangeData;

/**
 * Editor subsystem that handles remote sync notifications for LiveConfig
 * This keeps all editor UI logic separate from the runtime LiveConfigSystem
 */
UCLASS()
class LIVECONFIGEDITOR_API ULiveConfigEditorSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	void MonitorCsv(const FString& Path);

private:
	void EndMonitorCsv();
	
	void UpdateCsvMonitor();
	
	void HandleLiveConfigSettingsChanged(UObject* Settings, FPropertyChangedEvent& PropertyChangedEvent);
	
	/** Handles sync detection from LiveConfigSystem */
	void HandleSyncDetected(const TArray<FLiveConfigPropertyDefinition>& ChangedProps);

	/** Shows notification for AlwaysSync mode */
	void ShowSyncedChangesNotification(const TArray<FLiveConfigPropertyDefinition>& ChangedProps);

	/** Shows interactive prompt for Prompt mode */
	void ShowSyncPromptNotification(const TArray<FLiveConfigPropertyDefinition>& ChangedProps);

	/** Callback for directory changes */
	void OnFileChanged(const TArray<FFileChangeData>& FileChanges);

	TWeakPtr<SNotificationItem> SyncNotification;
	
	TArray<FLiveConfigPropertyDefinition> LastIgnoredChanges;

	FDelegateHandle FileWatcherHandle;
	FString TargetCsvPath;
};
