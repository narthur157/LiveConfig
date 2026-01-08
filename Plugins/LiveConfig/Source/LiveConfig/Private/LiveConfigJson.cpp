// All rights reserved - Genpop

#include "LiveConfigJson.h"
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
	LoadJsonFromFiles();
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
			if (ULiveConfigGameSettings* Settings = GetMutableDefault<ULiveConfigGameSettings>())
			{
				Settings->PropertyDefinitions.Add(PropertyDefinition.PropertyName, PropertyDefinition);
			}
		}
	}
}

void ULiveConfigJsonSystem::SaveJsonToFiles()
{
	if (ULiveConfigGameSettings* Settings = GetMutableDefault<ULiveConfigGameSettings>())
	{
		// 1. Collect all current property paths
		TSet<FString> CurrentPropertyPaths;
		for (const auto& Pair : Settings->PropertyDefinitions)
		{
			// Skip properties imported from curve tables
			static const FName FromCurveTableTag = TEXT("FromCurveTable");
			if (Pair.Value.Tags.Contains(FromCurveTableTag))
			{
				continue;
			}

			CurrentPropertyPaths.Add(FPaths::ConvertRelativePathToFull(GetPropertyPath(Pair.Value.PropertyName.GetName())));
			SavePropertyToFile(Pair.Value);
		}

		// 2. Clean up files that are no longer in the map
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
}

void ULiveConfigJsonSystem::SavePropertyToFile(const FLiveConfigPropertyDefinition& PropertyDefinition)
{
	// Skip properties imported from curve tables
	static const FName FromCurveTableTag = TEXT("FromCurveTable");
	if (PropertyDefinition.Tags.Contains(FromCurveTableTag))
	{
		return;
	}

	FString ActualPath = GetPropertyPath(PropertyDefinition.PropertyName.GetName());

	TSharedPtr<FJsonObject> JsonObject = FJsonObjectConverter::UStructToJsonObject(PropertyDefinition);
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
}

void ULiveConfigJsonSystem::DeletePropertyFile(FName PropertyName)
{
	FString ActualPath = GetPropertyPath(PropertyName);
	if (FPaths::FileExists(ActualPath))
	{
		if (IFileManager::Get().Delete(*ActualPath))
		{
			UE_LOG(LogLiveConfig, Log, TEXT("Deleted property file %s"), *ActualPath);
			PendingCheckoutFiles.Add(ActualPath);
		}
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
