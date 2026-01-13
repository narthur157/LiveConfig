// All rights reserved - Genpop

#include "LiveConfigJson.h"
#include "LiveConfigSystem.h"
#include "LiveConfigGameSettings.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObjectConverter.h"
#include "HAL/FileManager.h"

#if WITH_EDITOR
#include "SourceControlHelpers.h"
#include "SourceControlOperations.h"
#include "ISourceControlProvider.h"
#include "ISourceControlModule.h"
#endif

#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarVerifyJsonOnSave(
	TEXT("LiveConfig.VerifyJsonOnSave"),
	0,
	TEXT("If enabled, verify JSON integrity of data vs. folders/files on every save.\n0: Disabled\n1: Enabled"),
	ECVF_Default);

ULiveConfigJsonSystem* ULiveConfigJsonSystem::Get()
{
	if (!GEngine)
	{
		return nullptr;
	}

	return GEngine->GetEngineSubsystem<ULiveConfigJsonSystem>();
}

void ULiveConfigJsonSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	// ULiveConfigSystem will call this itself
	//	LoadJsonFromFiles();

	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("LiveConfig.VerifyJson"),
		TEXT("Verify JSON integrity of data vs. folders/files."),
		FConsoleCommandDelegate::CreateUObject(this, &ULiveConfigJsonSystem::VerifyJsonIntegrity),
		ECVF_Default
	);
}

void ULiveConfigJsonSystem::LoadJsonFromFiles()
{
	FString Dir = GetLiveConfigDirectory();
	TArray<FString> Files;
	if (IFileManager::Get().DirectoryExists(*Dir))
	{
		IFileManager::Get().FindFiles(Files, *Dir, TEXT("*.json"));
	}


	LoadJsonFromDirectory(Dir);

#if WITH_EDITOR
	if (PendingCheckoutFiles.Num() > 0)
	{
		SourceControlHelpers::CheckOutOrAddFiles(PendingCheckoutFiles);
		SourceControlHelpers::RevertUnchangedFiles(PendingCheckoutFiles);
		PendingCheckoutFiles.Empty();
	}
#endif 
}

void ULiveConfigJsonSystem::LoadJsonFromDirectory(const FString& Dir)
{
	TArray<FString> Dirs;
	TArray<FString> Files;
	FString SearchPath = Dir / TEXT("*");
	IFileManager::Get().FindFiles(Dirs, *SearchPath, false, true);
	IFileManager::Get().FindFiles(Files, *SearchPath, true, false);

	for (const FString& File : Files)
	{
		if (!File.EndsWith(TEXT(".json")))
		{
			continue;
		}
		
		LoadJsonFromFile(Dir, File);				
	}

	for (const FString& IterDir : Dirs)
	{
		LoadJsonFromDirectory(Dir / IterDir);
	}	
}

void ULiveConfigJsonSystem::LoadJsonFromFile(const FString& Path, const FString& FileName)
{
	FString FullPath = Path / FileName;
	if(!FPaths::FileExists(FullPath))
	{
		return;
	}

	FString JsonString;
	FFileHelper::LoadFileToString(JsonString, *FullPath);

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		FLiveConfigPropertyDefinition PropertyDefinition;
		if (FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), &PropertyDefinition))
		{
			if (ULiveConfigSystem* System = ULiveConfigSystem::Get())
			{
				System->PropertyDefinitions.Add(PropertyDefinition.PropertyName, PropertyDefinition);
			}
		}
	}
}

void ULiveConfigJsonSystem::VerifyJsonIntegrity()
{
	ULiveConfigGameSettings* Settings = GetMutableDefault<ULiveConfigGameSettings>();
	if (!Settings)
	{
		return;
	}

	FString LiveConfigDir = GetLiveConfigDirectory();
	TArray<FString> AllJsonFiles;
	IFileManager::Get().FindFilesRecursive(AllJsonFiles, *LiveConfigDir, TEXT("*.json"), true, false);

	TSet<FString> FilesOnDisk;
	for (const FString& FilePath : AllJsonFiles)
	{
		FilesOnDisk.Add(FPaths::ConvertRelativePathToFull(FilePath));
	}

	TSet<FString> ExpectedFiles;
	int32 MissingFilesCount = 0;
	int32 TotalProperties = 0;

	const ULiveConfigSystem* System = ULiveConfigSystem::Get();

	for (const auto& Pair : System->PropertyDefinitions)
	{
		if (Pair.Value.Tags.Contains(LiveConfigTags::FromCurveTable))
		{
			continue;
		}

		TotalProperties++;
		FString ExpectedPath = FPaths::ConvertRelativePathToFull(GetPropertyPath(Pair.Value.PropertyName.GetName()));
		ExpectedFiles.Add(ExpectedPath);

		if (!FilesOnDisk.Contains(ExpectedPath))
		{
			UE_LOG(LogLiveConfig, Warning, TEXT("Integrity Check: Missing JSON file for property %s. Expected at: %s"), *Pair.Value.PropertyName.ToString(), *ExpectedPath);
			MissingFilesCount++;
		}
	}

	int32 OrphanedFilesCount = 0;
	for (const FString& FilePath : FilesOnDisk)
	{
		if (!ExpectedFiles.Contains(FilePath))
		{
			UE_LOG(LogLiveConfig, Warning, TEXT("Integrity Check: Orphaned JSON file found: %s"), *FilePath);
			OrphanedFilesCount++;
		}
	}

	if (MissingFilesCount == 0 && OrphanedFilesCount == 0)
	{
		UE_LOG(LogLiveConfig, Log, TEXT("Integrity Check Passed: %d properties verified, no discrepancies found."), TotalProperties);
	}
	else
	{
		UE_LOG(LogLiveConfig, Error, TEXT("Integrity Check Failed: %d Missing Files, %d Orphaned Files."), MissingFilesCount, OrphanedFilesCount);
	}
}

void ULiveConfigJsonSystem::SaveJsonToFiles()
{
	if (const ULiveConfigSystem* System = ULiveConfigSystem::Get())
	{
		// Collect all current property paths
		TSet<FString> CurrentPropertyPaths;
		for (const auto& Pair : System->PropertyDefinitions)
		{
			// Skip properties imported from curve tables
			if (Pair.Value.Tags.Contains(LiveConfigTags::FromCurveTable))
			{
				continue;
			}

			CurrentPropertyPaths.Add(FPaths::ConvertRelativePathToFull(GetPropertyPath(Pair.Value.PropertyName.GetName())));
			SavePropertyToFile(Pair.Value);
		}

		// Clean up files that are no longer in the map
		TArray<FString> AllFiles;
		IFileManager::Get().FindFilesRecursive(AllFiles, *GetLiveConfigDirectory(), TEXT("*.json"), true, false);

		for (const FString& FilePath : AllFiles)
		{
			FString FullFilePath = FPaths::ConvertRelativePathToFull(FilePath);
			if (!CurrentPropertyPaths.Contains(FullFilePath))
			{
				if (IFileManager::Get().Delete(*FullFilePath))
				{
					UE_LOG(LogLiveConfig, Log, TEXT("Deleted obsolete property file: %s"), *FullFilePath);
#if WITH_EDITOR
					PendingCheckoutFiles.Add(FullFilePath);
#endif
				}
			}
		}
	}
	if (CVarVerifyJsonOnSave.GetValueOnGameThread() != 0)
	{
		VerifyJsonIntegrity();
	}
}

void ULiveConfigJsonSystem::SavePropertyToFile(const FLiveConfigPropertyDefinition& PropertyDefinition)
{
	if (!PropertyDefinition.PropertyName.IsValid() || PropertyDefinition.PropertyName.ToString().EndsWith(TEXT(".")))
	{
		return;
	}

	// Skip properties imported from curve tables
	if (PropertyDefinition.Tags.Contains(LiveConfigTags::FromCurveTable))
	{
		return;
	}

	FString ActualPath = GetPropertyPath(PropertyDefinition.PropertyName.GetName());

	FLiveConfigPropertyDefinition DefCopy = PropertyDefinition;
	if (DefCopy.PropertyType == ELiveConfigPropertyType::Float && !DefCopy.Value.IsEmpty())
	{
		float FloatVal = FCString::Atof(*DefCopy.Value);
		DefCopy.Value = FString::SanitizeFloat(FloatVal);
	}

	TSharedPtr<FJsonObject> JsonObject = FJsonObjectConverter::UStructToJsonObject(DefCopy);
	if (JsonObject.IsValid())
	{
		FString JsonString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
		FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

		if (FFileHelper::SaveStringToFile(JsonString, *ActualPath))
		{
			UE_LOG(LogLiveConfig, Log, TEXT("Saved property %s to %s"), *PropertyDefinition.PropertyName.ToString(), *ActualPath);
		}
		else
		{
			UE_LOG(LogLiveConfig, Error, TEXT("Failed to save property %s to %s"), *PropertyDefinition.PropertyName.ToString(), *ActualPath);
		}

		PendingCheckoutFiles.Add(ActualPath);
	}

	if (CVarVerifyJsonOnSave.GetValueOnGameThread() != 0)
	{
		VerifyJsonIntegrity();
	}
}

void ULiveConfigJsonSystem::DeletePropertyFile(FName PropertyName)
{
	if (PropertyName.IsNone() || PropertyName.ToString().EndsWith(TEXT(".")))
	{
		return;
	}

	FString ActualPath = GetPropertyPath(PropertyName);
	if (FPaths::FileExists(ActualPath))
	{
		if (IFileManager::Get().Delete(*ActualPath))
		{
			UE_LOG(LogLiveConfig, Log, TEXT("Deleted property file %s"), *ActualPath);
			PendingCheckoutFiles.Add(ActualPath);
		}
	}

	if (CVarVerifyJsonOnSave.GetValueOnGameThread() != 0)
	{
		VerifyJsonIntegrity();
	}
}

void ULiveConfigJsonSystem::CheckoutProperties(const TArray<FName>& PropertyNames)
{
#if WITH_EDITOR
	TArray<FString> Paths;
	for (FName PropertyName : PropertyNames)
	{
		Paths.Add(GetPropertyPath(PropertyName));
	}
	
	SourceControlHelpers::CheckOutFiles(Paths);
#endif
}

FString ULiveConfigJsonSystem::GetPropertyPath(FName PropertyName)
{
	FString PropertyPath = PropertyName.ToString().Replace(TEXT("."), TEXT("/"));
	FString ActualPath = GetLiveConfigDirectory() / PropertyPath + TEXT(".json");
	return ActualPath;
}

FString ULiveConfigJsonSystem::GetLiveConfigDirectory()
{
	return FPaths::ProjectConfigDir() / TEXT("LiveConfig");
}
