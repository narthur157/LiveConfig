// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#include "LiveConfigEditorSubsystem.h"
#include "LiveConfigSystem.h"
#include "LiveConfigUserSettings.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "DirectoryWatcherModule.h"
#include "IDirectoryWatcher.h"
#include "LiveConfigSettings.h"

void ULiveConfigEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ULiveConfigSystem::Get().OnDataProviderSync.AddUObject(this, &ULiveConfigEditorSubsystem::HandleSyncDetected);

	// Setup file watcher for local CSV
	ELiveConfigSourceType SourceType;
	ULiveConfigSystem::GetActiveSource(SourceType, TargetCsvPath);

	if (SourceType == ELiveConfigSourceType::LocalCsv && !TargetCsvPath.IsEmpty())
	{
		MonitorCsv(TargetCsvPath);
	}
	
	GetMutableDefault<ULiveConfigUserSettings>()->OnSettingChanged().AddUObject(this, &ThisClass::HandleLiveConfigSettingsChanged);
	GetMutableDefault<ULiveConfigSettings>()->OnSettingChanged().AddUObject(this, &ThisClass::HandleLiveConfigSettingsChanged);
}

void ULiveConfigEditorSubsystem::Deinitialize()
{
	EndMonitorCsv();	

	// Unbind from delegate
	if (ULiveConfigSystem* System = GEngine->GetEngineSubsystem<ULiveConfigSystem>())
	{
		System->OnDataProviderSync.RemoveAll(this);
	}

	Super::Deinitialize();
}

void ULiveConfigEditorSubsystem::MonitorCsv(const FString& Path)
{
	if (Path.IsEmpty())
	{
		return;
	}
	
	if (!TargetCsvPath.IsEmpty())
	{
		EndMonitorCsv();	
	}
	
	FDirectoryWatcherModule& DirectoryWatcherModule = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>("DirectoryWatcher");

	if (IDirectoryWatcher* DirectoryWatcher = DirectoryWatcherModule.Get())
	{
		FString Directory = FPaths::GetPath(Path);
		DirectoryWatcher->RegisterDirectoryChangedCallback_Handle(
			Directory,
			IDirectoryWatcher::FDirectoryChanged::CreateUObject(this, &ULiveConfigEditorSubsystem::OnFileChanged),
			FileWatcherHandle,
			IDirectoryWatcher::WatchOptions::IgnoreChangesInSubtree
		);
	}
}

void ULiveConfigEditorSubsystem::EndMonitorCsv()
{
	// Unregister file watcher
	if (FileWatcherHandle.IsValid())
	{
		FDirectoryWatcherModule* DirectoryWatcherModule = FModuleManager::GetModulePtr<FDirectoryWatcherModule>("DirectoryWatcher");
		if (DirectoryWatcherModule)
		{
			IDirectoryWatcher* DirectoryWatcher = DirectoryWatcherModule->Get();
			if (DirectoryWatcher)
			{
				FString Directory = FPaths::GetPath(TargetCsvPath);
				DirectoryWatcher->UnregisterDirectoryChangedCallback_Handle(Directory, FileWatcherHandle);
			}
		}
	}
}

void ULiveConfigEditorSubsystem::UpdateCsvMonitor()
{
	ELiveConfigSourceType SourceType;
	FString SourcePath;
	
	ULiveConfigSystem::Get().GetActiveSource(SourceType, SourcePath);
	
	if (SourceType == ELiveConfigSourceType::LocalCsv)
	{
		MonitorCsv(SourcePath);
	}
	else
	{
		EndMonitorCsv();
	}
}

void ULiveConfigEditorSubsystem::HandleLiveConfigSettingsChanged(UObject* Settings,
	FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.MemberProperty->GetName() == GET_MEMBER_NAME_CHECKED(ULiveConfigUserSettings, ExternalSourceOverride) 
		|| PropertyChangedEvent.MemberProperty->GetName() == GET_MEMBER_NAME_CHECKED(ULiveConfigSettings, DefaultExternalSource))
	{
		UpdateCsvMonitor();
	}
}

void ULiveConfigEditorSubsystem::OnFileChanged(const TArray<FFileChangeData>& FileChanges)
{
	for (const FFileChangeData& Change : FileChanges)
	{
		UE_LOG(LogTemp, Log, TEXT("Change type %d file %s"), Change.Action, *Change.Filename);
		bool bValidChangeType = Change.Action == FFileChangeData::FCA_Modified || Change.Action == FFileChangeData::FCA_Added;
		if (bValidChangeType && FPaths::ConvertRelativePathToFull(Change.Filename) == FPaths::ConvertRelativePathToFull(TargetCsvPath))
		{
			UE_LOG(LogLiveConfig, Log, TEXT("Detected change in CSV: %s. Triggering sync..."), *TargetCsvPath);
			ULiveConfigSystem::Get().SyncRemoteToLocal();
			break;
		}
	}
}

void ULiveConfigEditorSubsystem::HandleSyncDetected(const TArray<FLiveConfigPropertyDefinition>& ChangedProps)
{
	if (ChangedProps.Num() == 0)
	{
		return;
	}

	ELiveConfigSyncMode SyncMode = GetDefault<ULiveConfigUserSettings>()->SyncMode;

	switch (SyncMode)
	{
	case ELiveConfigSyncMode::AlwaysSync:
		ULiveConfigSystem::Get().SaveProperties(ChangedProps);	
		ShowSyncedChangesNotification(ChangedProps);
		break;
	case ELiveConfigSyncMode::Prompt:
		ShowSyncPromptNotification(ChangedProps);
		break;
	case ELiveConfigSyncMode::NeverSync:
		break;
	}
}

void ULiveConfigEditorSubsystem::ShowSyncedChangesNotification(const TArray<FLiveConfigPropertyDefinition>& ChangedProps)
{
	FText DiffText = ULiveConfigSystem::Get().GetDiffText(ChangedProps);
	
	FNotificationInfo Info(FText::Format(
		NSLOCTEXT("LiveConfig", "ChangesSynced", "Synced {0} remote change(s)\n{1}"),
		FText::AsNumber(ChangedProps.Num()),
		DiffText
	));

	Info.ExpireDuration = 5.0f;
	Info.bFireAndForget = true;

	SyncNotification = FSlateNotificationManager::Get().AddNotification(Info);
	if (SyncNotification.IsValid())
	{
		SyncNotification.Pin()->SetCompletionState(SNotificationItem::CS_Success);
	}
}

void ULiveConfigEditorSubsystem::ShowSyncPromptNotification(const TArray<FLiveConfigPropertyDefinition>& ChangedProps)
{
	bool bSameAsIgnored = true;
	for (int32 Index = 0; Index < ChangedProps.Num(); Index++)
	{
		if (!LastIgnoredChanges.IsValidIndex(Index))
		{
			bSameAsIgnored = false;	
			break;
		}
		const FLiveConfigPropertyDefinition& ChangedDef = ChangedProps[Index];
		const auto& IgnoredDef = LastIgnoredChanges[Index];
		
		if (ChangedDef.PropertyName != IgnoredDef.PropertyName || ChangedDef.Value != IgnoredDef.Value)
		{
			bSameAsIgnored = false;
			break;
		}
	}
	
	if (bSameAsIgnored)
	{
		return;
	}
	
	if (SyncNotification.IsValid())
	{
		SyncNotification.Pin()->ExpireAndFadeout();
	}

	FText DiffText = ULiveConfigSystem::Get().GetDiffText(ChangedProps);

	FNotificationInfo Info(FText::Format(
		NSLOCTEXT("LiveConfig", "SyncRemoteChangesPrompt", "Detected {0} remote change(s). Would you like to apply them to local project settings?\n{1}"),
		FText::AsNumber(ChangedProps.Num()),
		DiffText
	));

	Info.bFireAndForget = false;
	Info.FadeOutDuration = 0.0f;
	Info.ExpireDuration = 0.0f;

	Info.ButtonDetails.Add(FNotificationButtonInfo(
		NSLOCTEXT("LiveConfig", "ApplyButtonText", "Apply"),
		FText::GetEmpty(),
		FSimpleDelegate::CreateWeakLambda(this, [this, ChangedProps]()
		{
			if (SyncNotification.IsValid())
			{
				SyncNotification.Pin()->SetCompletionState(SNotificationItem::CS_Success);
				SyncNotification.Pin()->ExpireAndFadeout();
				SyncNotification.Reset();
			}

			ULiveConfigSystem::Get().SaveProperties(ChangedProps);
			ShowSyncedChangesNotification(ChangedProps);
		}),
		SNotificationItem::CS_Pending
	));

	Info.ButtonDetails.Add(FNotificationButtonInfo(
		NSLOCTEXT("LiveConfig", "IgnoreButtonText", "Ignore"),
		FText::GetEmpty(),
		FSimpleDelegate::CreateWeakLambda(this, [this, ChangedProps]()
		{
			if (SyncNotification.IsValid())
			{
				SyncNotification.Pin()->ExpireAndFadeout();
				SyncNotification.Reset();
			}
			
			LastIgnoredChanges = ChangedProps;
		}),
		SNotificationItem::CS_Pending
	));

	SyncNotification = FSlateNotificationManager::Get().AddNotification(Info);
	if (SyncNotification.IsValid())
	{
		SyncNotification.Pin()->SetCompletionState(SNotificationItem::CS_Pending);
	}
}
