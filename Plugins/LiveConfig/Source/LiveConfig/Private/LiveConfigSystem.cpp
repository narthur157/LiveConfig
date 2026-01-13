#include "LiveConfigSystem.h"

#include "Profiles/LiveConfigProfileSystem.h"
#include "HttpModule.h"
#include "LiveConfigEditorSettings.h"
#include "LiveConfigGameSettings.h"
#include "Interfaces/IHttpResponse.h"
#include "LiveConfigTypes.h"
#include "Serialization/Csv/CsvParser.h"

#include "LiveConfigJson.h"
#include "ConsoleSettings.h"
#include "Engine/Console.h"

namespace LiveConfigTags
{
	const FName FromCurveTable = TEXT("FromCurveTable");
}

ULiveConfigSystem* ULiveConfigSystem::Get()
{
    return GEngine->GetEngineSubsystem<ULiveConfigSystem>();
}

void ULiveConfigSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Collection.InitializeDependency(ULiveConfigProfileSystem::StaticClass());
    Collection.InitializeDependency(ULiveConfigJsonSystem::StaticClass());
    
    SheetUrl = GetDefault<ULiveConfigGameSettings>()->SheetUrl;

    UConsole::RegisterConsoleAutoCompleteEntries.AddUObject(this, &ThisClass::PopulateAutoCompleteEntries);

    if (ULiveConfigJsonSystem* JsonSystem = ULiveConfigJsonSystem::Get())
    {
        JsonSystem->LoadJsonFromFiles();
    }

    RefreshFromSettings();

   	// Register "FromCurveTable" as a known tag if it's not already there
   	if (ULiveConfigGameSettings* Settings = GetMutableDefault<ULiveConfigGameSettings>())
   	{
   		Settings->KnownTags.AddUnique(LiveConfigTags::FromCurveTable);
   	}

    DownloadConfig();

    FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &ThisClass::OnTravel);
    FWorldDelegates::OnStartGameInstance.AddUObject(this, &ThisClass::OnStartGameInstance);

    if (ULiveConfigProfileSystem* ProfileSystem = ULiveConfigProfileSystem::Get())
    {
        ProfileSystem->OnProfileChanged.AddUObject(this, &ThisClass::RefreshFromSettings);
    }
}

void ULiveConfigSystem::RefreshFromSettings()
{
    BuildCache();
    OnPropertiesUpdated.Broadcast();
}

void ULiveConfigSystem::RefreshFromSettings(const FLiveConfigProfile& Profile)
{
    RefreshFromSettings();
}

void ULiveConfigSystem::Deinitialize()
{
    Super::Deinitialize();
}

bool ULiveConfigSystem::ShouldCreateSubsystem(UObject* Outer) const
{
    return true;
}

void ULiveConfigSystem::DownloadConfig()
{
    if (IsRunningCookCommandlet())
    {
        UE_LOG(LogLiveConfig, Log, TEXT("Ignoring live config during cooking"));
        return;
    }
 
    if (CurrentRequest && !CurrentRequest->GetResponse())
    {
        UE_LOG(LogLiveConfig, Log, TEXT("Live config request already in progress, not downloading"));
        return;
    }

    double TimeSinceLastDownload = FPlatformTime::Seconds() - TimeLoadStarted;
    if (TimeSinceLastDownload < RateLimitSeconds)
    {
        UE_LOG(LogLiveConfig, Log, TEXT("Preventing downloading config due to rate limit, time since last download %f (limit %f)"), TimeSinceLastDownload, RateLimitSeconds);
        return;
    }
    
    // need to sanitize sheet url so that it definitely has /export?format=csv
    // we may or may not want to specify the GID, depending on if the tab we're using is the first or not
    // could have GID as an optional arg, but this also gets confusing? Just going to assume it's the first tab for now
    if (SheetUrl.IsEmpty())
    {
        UE_LOG(LogLiveConfig, Warning, TEXT("LiveConfigSystem: SheetUrl is empty. Please set it in DefaultGame.ini."));
        return;
    }

    FHttpModule& HttpModule = FHttpModule::Get();
    CurrentRequest = HttpModule.CreateRequest();

    CurrentRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnSheetDownloadComplete);
    CurrentRequest->SetURL(SheetUrl);
    CurrentRequest->SetVerb(TEXT("GET"));
    CurrentRequest->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
    CurrentRequest->SetHeader(TEXT("Content-Type"), TEXT("text/csv"));
    
    UE_LOG(LogLiveConfig, Log, TEXT("LiveConfigSystem: Starting download from %s"), *SheetUrl);
    CurrentRequest->ProcessRequest();
    TimeLoadStarted = FPlatformTime::Seconds();
}

void ULiveConfigSystem::OnSheetDownloadComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogLiveConfig, Error, TEXT("LiveConfigSystem: Download failed."));
        return;
    }

    ULiveConfigGameSettings* GameSettings = GetMutableDefault<ULiveConfigGameSettings>();
    if (!GameSettings)
    {
        return;
    }

    const FString CsvContent = Response->GetContentAsString();
    
    FCsvParser Parser(CsvContent);
    const FCsvParser::FRows& Rows = Parser.GetRows();

    FLiveConfigProfile NewEnvProfile;
    // Start from index 1 to skip the header row
    for (int32 i = 1; i < Rows.Num(); ++i)
    {
        const TArray<const TCHAR*>& Columns = Rows[i];

        // We still expect at least four columns: key, value, tags, description
        if (Columns.Num() >= 4)
        {
            // The parser gives us TCHAR*, so we convert them to FName/FString
            const FName Key(Columns[0]);
            
            // Try to find existing definition to preserve its metadata if needed, 
            // but actually we want to override values from the sheet.
            if (!PropertyDefinitions.Contains(Key))
            {
                UE_LOG(LogLiveConfig, Warning, TEXT("Skipping remote property with invalid key: %s"), *Key.ToString());
                continue;
            }
            
            FLiveConfigPropertyDefinition Def = PropertyDefinitions[Key];
            Def.PropertyName = Key;
            Def.Value = Columns[1];
            
            // If it's a float, sanitize it to float precision to avoid double precision artifacts
            if (Def.PropertyType == ELiveConfigPropertyType::Float && !Def.Value.IsEmpty())
            {
                float FloatVal = FCString::Atof(*Def.Value);
                Def.Value = FString::SanitizeFloat(FloatVal);
            }
            
            if (Key != NAME_None && !Def.Value.IsEmpty())
            {
                UE_LOG(LogLiveConfig, Verbose, TEXT("Downloaded config value: %s: %s (%s)"), *Key.ToString(), *Def.Value, *Def.Description);
                NewEnvProfile.Overrides.Add(Def.PropertyName, Def.Value);
            }
        }
    }
    
    bIsDataReady = true;
    UE_LOG(LogLiveConfig, Log, TEXT("Successfully loaded %d key-value pairs"), PropertyDefinitions.Num());
    
    if (NewEnvProfile != EnvironmentOverrides)
    {   
        UE_LOG(LogLiveConfig, Log, TEXT("LiveConfigSystem:Environment profile changed, rebuilding cache"));
        EnvironmentOverrides = NewEnvProfile;
        RefreshFromSettings();
    }
}

const TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition>& ULiveConfigSystem::GetAllProperties() const
{
    return PropertyDefinitions;
}

bool ULiveConfigSystem::DoesPropertyNameExist(FLiveConfigProperty PropertyName)
{
    if (ULiveConfigSystem* System = ULiveConfigSystem::Get())
    {
        return System->PropertyDefinitions.Contains(PropertyName);
    }
    return false;
}

void ULiveConfigSystem::OnTravel(UWorld* World, FWorldInitializationValues WorldInitializationValues)
{
    DownloadConfig();
}

void ULiveConfigSystem::OnStartGameInstance(UGameInstance* GameInstance)
{
    float PollingRate = ULiveConfigGameSettings::StaticClass()->GetDefaultObject<ULiveConfigGameSettings>()->PollingRate;
    
#if WITH_EDITOR
    PollingRate = ULiveConfigEditorSettings::StaticClass()->GetDefaultObject<ULiveConfigEditorSettings>()->EditorPollRateMinutes * 60;
#endif
    
    GameInstance->GetTimerManager().SetTimer(PollingTimer, FTimerDelegate::CreateWeakLambda(this, [&]
    {
        DownloadConfig();
    }), PollingRate, true);
}

void ULiveConfigSystem::BuildCache()
{
    ULiveConfigProfileSystem* ProfileSystem = ULiveConfigProfileSystem::Get();
    
    if (ProfileSystem)
    {
        FLiveConfigCache::BuildConfig(PropertyDefinitions, EnvironmentOverrides, ProfileSystem->GetActiveProfile(), Cache);
    }
    else
    {
        FLiveConfigCache::BuildConfig(PropertyDefinitions, EnvironmentOverrides, {}, Cache);
    }
}

FString ULiveConfigSystem::GetStringValue(FLiveConfigProperty Key)
{
    return Cache.GetValue<FString>(Key);
}

float ULiveConfigSystem::GetFloatValue(FLiveConfigProperty Key)
{
    return Cache.GetValue<float>(Key);
}

int32 ULiveConfigSystem::GetIntValue(FLiveConfigProperty Key)
{
    return Cache.GetValue<int32>(Key);
}

bool ULiveConfigSystem::GetBoolValue(FLiveConfigProperty Key)
{
    return Cache.GetValue<bool>(Key);
}

void ULiveConfigSystem::PopulateAutoCompleteEntries(TArray<FAutoCompleteCommand>& Entries)
{
    // Use iterator to avoid copying the full property list 
    for (auto Iter = PropertyDefinitions.CreateConstIterator(); Iter; ++Iter)
    {
        const FLiveConfigProperty& Prop = Iter->Key;
        FAutoCompleteCommand Command;
        Command.Command = FString::Printf(TEXT("LiveConfig.SetOverride %s"), *Prop.ToString());
        Command.Desc = FString::Printf(TEXT("Set live config override for %s (current %s)"), *Prop.ToString(), *GetStringValue(Prop));
        Entries.Add(Command);
    }
}
