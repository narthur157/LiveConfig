#include "Profiles/LiveConfigProfileSystem.h"
#include "LiveConfigSystem.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "JsonObjectConverter.h"
#include "LiveConfigGameSettings.h"
#include "LiveConfigLib.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/IConsoleManager.h"
#include "Engine/Console.h"
#include "Engine/Engine.h"
#include "Profiles/LiveConfigProfileActor.h"
#include "Profiles/LiveConfigProfileReplicationSystem.h"

DEFINE_LOG_CATEGORY(LogLiveConfigProfile);

ULiveConfigProfileSystem* ULiveConfigProfileSystem::Get()
{
    return GEngine ? GEngine->GetEngineSubsystem<ULiveConfigProfileSystem>() : nullptr;
}

void ULiveConfigProfileSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UConsole::RegisterConsoleAutoCompleteEntries.AddUObject(this, &ThisClass::PopulateAutoCompleteEntries);

    IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("LiveConfig.SaveProfile"),
        TEXT("Saves the active profile to disk with the given name. Usage: LiveConfig.SaveProfile ProfileName"),
        FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& Args)
        {
            if (Args.Num() > 0)
            {
                ActiveProfile.ProfileName = *Args[0];
                SaveProfile(ActiveProfile);
                UE_LOG(LogLiveConfigProfile, Log, TEXT("Saved profile: %s"), *Args[0]);
            }
        }),
        ECVF_Default
    );

    IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("LiveConfig.LoadProfile"),
        TEXT("Loads a profile from disk and sets it as active. Usage: LiveConfig.LoadProfile ProfileName"),
        FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& Args)
        {
            if (Args.Num() > 0)
            {
                SetActiveProfile(*Args[0]);
                UE_LOG(LogLiveConfigProfile, Log, TEXT("Loaded profile: %s"), *Args[0]);
            }
        }),
        ECVF_Default
    );

    IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("LiveConfig.SetOverride"),
        TEXT("Sets an override in the active profile. Usage: LiveConfig.SetOverride PropertyName Value"),
        FConsoleCommandWithArgsDelegate::CreateLambda([this](const TArray<FString>& Args)
        {
            if (Args.Num() > 1 && !Args[0].IsEmpty() && !Args[1].IsEmpty())
            {
                FLiveConfigProperty Prop = FLiveConfigProperty(Args[0]);
                if (!ULiveConfigSystem::Get()->DoesPropertyNameExist(Prop))
                {
                    UE_LOG(LogLiveConfig, Warning, TEXT("Property '%s' does not exist in the system. Override not set."), *Args[0]);
                    return;
                }
                
                ActiveProfile.Overrides.Add(Prop, Args[1]);
                SetActiveProfileData(ActiveProfile);
                UE_LOG(LogLiveConfigProfile, Log, TEXT("Set override: %s = %s"), *Args[0], *Args[1]);
            }
            else
            {
                UE_LOG(LogLiveConfig, Warning, TEXT("Invalid arguments for LiveConfig.SetOverride. Usage: LiveConfig.SetOverride PropertyName Value"));
            }
        }),
        ECVF_Default
    );

    IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("LiveConfig.ClearOverrides"),
        TEXT("Clears all overrides in the active profile."),
        FConsoleCommandDelegate::CreateLambda([this]()
        {
            ActiveProfile.Overrides.Empty();
            OnProfileChanged.Broadcast(ActiveProfile);
            UE_LOG(LogLiveConfigProfile, Log, TEXT("Cleared overrides"));
        }),
        ECVF_Default
    );

    IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("LiveConfig.ListProfiles"),
        TEXT("Lists all available profiles."),
        FConsoleCommandDelegate::CreateLambda([this]()
        {
            TArray<FName> Profiles = GetAllProfileNames();
            UE_LOG(LogLiveConfigProfile, Log, TEXT("Available profiles:"));
            for (FName Profile : Profiles)
            {
                UE_LOG(LogLiveConfigProfile, Log, TEXT("  %s"), *Profile.ToString());
            }
        }),
        ECVF_Default
    );

    IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("LiveConfig.ShowActiveProfile"),
        TEXT("Shows the active profile and its overrides."),
        FConsoleCommandDelegate::CreateLambda([this]()
        {
            UE_LOG(LogLiveConfigProfile, Log, TEXT("Active Profile: %s"), *ActiveProfile.ProfileName.ToString());
            for (auto& Pair : ActiveProfile.Overrides)
            {
                UE_LOG(LogLiveConfigProfile, Log, TEXT("  %s: %s"), *Pair.Key.ToString(), *Pair.Value);
            }
        }),
        ECVF_Default
    );
}

void ULiveConfigProfileSystem::SaveProfile(const FLiveConfigProfile& Profile)
{
    if (!Profile.IsValid())
    {
        return;
    }

    FString Path = GetProfilePath(Profile.ProfileName);
    TSharedPtr<FJsonObject> JsonObject = FJsonObjectConverter::UStructToJsonObject(Profile);
    if (JsonObject.IsValid())
    {
        FString JsonString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
        FFileHelper::SaveStringToFile(JsonString, *Path);
    }
}

bool ULiveConfigProfileSystem::LoadProfile(FName ProfileName, FLiveConfigProfile& OutProfile)
{
    FString Path = GetProfilePath(ProfileName);
    
    if (!FPaths::FileExists(Path))
    {
        return false;
    }

    FString JsonString;
    if (FFileHelper::LoadFileToString(JsonString, *Path))
    {
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
        if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
        {
            return FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), &OutProfile);
        }
        
        UE_LOG(LogLiveConfig, Error, TEXT("Failed to deserialize JSON for profile %s"), *ProfileName.ToString()); 
    }
    
    UE_LOG(LogLiveConfig, Error, TEXT("Failed to load profile %s"), *ProfileName.ToString());
    
    return false;
}

void ULiveConfigProfileSystem::DeleteProfile(FName ProfileName)
{
    FString Path = GetProfilePath(ProfileName);
    if (FPaths::FileExists(Path))
    {
        IFileManager::Get().Delete(*Path);
    }
}

TArray<FName> ULiveConfigProfileSystem::GetAllProfileNames() const
{
    TArray<FName> Names;
    FString Dir = GetProfilesDirectory();
    TArray<FString> Files;
    IFileManager::Get().FindFiles(Files, *Dir, TEXT("*.json"));

    for (const FString& File : Files)
    {
        Names.Add(*FPaths::GetBaseFilename(File));
    }
    return Names;
}

void ULiveConfigProfileSystem::SetActiveProfile(FName ProfileName)
{
    if (ProfileName == NAME_None)
    {
        SetActiveProfileData(FLiveConfigProfile());
        return;
    }

    FLiveConfigProfile NewProfile;
    if (LoadProfile(ProfileName, NewProfile))
    {
        SetActiveProfileData(NewProfile);
    }
}

void ULiveConfigProfileSystem::SetActiveProfileData(const FLiveConfigProfile& Profile)
{
    // TODO: In what scenarios does current play world not suffice?
    if (UWorld* World = GEngine->GetCurrentPlayWorld())
    {
        if (World->GetNetMode() == NM_Standalone)
        {
            SetActiveProfileData_NoReplication(Profile);
        }
        else
        {
            bool bAttemptedReplication = ULiveConfigProfileReplicationSystem::ReplicateProfileData(Profile, World);
            
            if (!bAttemptedReplication)
            {
                UE_LOG(LogLiveConfig, Error, TEXT("Failed to replicate live config profile, no ALiveConfigProfileActor found! Setting it directly")); 
                SetActiveProfileData_NoReplication(Profile);
            }
        }
    }
    else
    {
        // In some cases we might not care about replicating, so we'll skip logging a warning here
        SetActiveProfileData_NoReplication(Profile);
    }
}

void ULiveConfigProfileSystem::SetActiveProfileData_NoReplication(const FLiveConfigProfile& Profile)
{
    ActiveProfile = Profile;
    OnProfileChanged.Broadcast(Profile);
}

FString ULiveConfigProfileSystem::GetProfilesDirectory()
{
    // These are in their own folder to prevent getting mixed up with a potential "LiveConfig.*" property
    return FPaths::ProjectConfigDir() / TEXT("LiveConfigProfiles");
}

FString ULiveConfigProfileSystem::GetProfilePath(FName ProfileName)
{
    return GetProfilesDirectory() / ProfileName.ToString() + TEXT(".json");
}

void ULiveConfigProfileSystem::PopulateAutoCompleteEntries(TArray<FAutoCompleteCommand>& Entries)
{
    if (ULiveConfigSystem* ConfigSystem = ULiveConfigSystem::Get())
    {
        if (const ULiveConfigGameSettings* GameSettings = GetDefault<ULiveConfigGameSettings>())
        {
            // Use iterator to avoid copying the full property list 
            for (auto Iter = GameSettings->PropertyDefinitions.CreateConstIterator(); Iter; ++Iter)
            {
                const FLiveConfigProperty& Prop = Iter->Key;
                FAutoCompleteCommand Command;
                Command.Command = FString::Printf(TEXT("LiveConfig.SetOverride %s"), *Prop.ToString());
                Command.Desc = FString::Printf(TEXT("Set live config override for %s (current %s)"), *Prop.ToString(), *ConfigSystem->GetStringValue(Prop));
                Entries.Add(Command);
            }
        }
    }
}
