#include "LiveConfigSystem.h"

#include "HttpModule.h"
#include "LiveConfigEditorSettings.h"
#include "LiveConfigGameSettings.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/Csv/CsvParser.h"

DEFINE_LOG_CATEGORY(LogLiveConfig);

ULiveConfigSystem* ULiveConfigSystem::Get()
{
    return GEngine->GetEngineSubsystem<ULiveConfigSystem>();
}

void ULiveConfigSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    SheetUrl = ULiveConfigGameSettings::StaticClass()->GetDefaultObject<ULiveConfigGameSettings>()->SheetUrl;

    DownloadConfig();

    FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &ThisClass::OnTravel);
    FWorldDelegates::OnStartGameInstance.AddUObject(this, &ThisClass::OnStartGameInstance);
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

void ULiveConfigSystem::CheckUpdate()
{
    DownloadConfig();
}

void ULiveConfigSystem::OnSheetDownloadComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogLiveConfig, Error, TEXT("LiveConfigSystem: Download failed."));
        return;
    }

    ConfigValues.Empty();

    const FString CsvContent = Response->GetContentAsString();
    
    FCsvParser Parser(CsvContent);
    const FCsvParser::FRows& Rows = Parser.GetRows();

    // Start from index 1 to skip the header row
    for (int32 i = 1; i < Rows.Num(); ++i)
    {
        const TArray<const TCHAR*>& Columns = Rows[i];

        // We still expect at least four columns: key, value, tags, description
        if (Columns.Num() >= 4)
        {
            // The parser gives us TCHAR*, so we convert them to FName/FString
            const FName Key(Columns[0]);
            FLiveConfigValue Value;
            Value.RawValue = Columns[1];
            Value.Description = Columns[3];
            
            if (Key != NAME_None && !Value.RawValue.IsEmpty())
            {
                ConfigValues.Add(Key, Value);
                UE_LOG(LogLiveConfig, Log, TEXT("Downloaded config value: %s: %s (%s)"), *Key.ToString(), *Value.RawValue, *Value.Description);
            }
        }
    }
    
    bIsDataReady = true;
    UE_LOG(LogLiveConfig, Log, TEXT("LiveConfigSystem:Successfully loaded %d key-value pairs"), ConfigValues.Num());
}

TArray<FLiveConfigProperty> ULiveConfigSystem::GetAllProperties() const
{
    TArray<FLiveConfigProperty> Properties;
    ConfigValues.GetKeys(Properties);
    return Properties;
}

bool ULiveConfigSystem::DoesPropertyNameExist(FLiveConfigProperty PropertyName) const
{
    return ConfigValues.Contains(PropertyName);
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

FString ULiveConfigSystem::GetStringValue(FLiveConfigProperty Key, const FString& DefaultValue)
{
    if (!ConfigValues.Contains(Key))
    {
        return DefaultValue;        
    }
    
    const FLiveConfigValue* FoundValue = ConfigValues.Find(Key);
    return FoundValue ? *FoundValue->RawValue : DefaultValue;
}

float ULiveConfigSystem::GetFloatValue(FLiveConfigProperty Key, float DefaultValue)
{
    if (!ConfigValues.Contains(Key))
    {
        return DefaultValue;        
    }
    
    const FString StringValue = GetStringValue(Key);
    if (!StringValue.IsEmpty())
    {
        return FCString::Atof(*StringValue);
    }

    return DefaultValue;
}

int32 ULiveConfigSystem::GetIntValue(FLiveConfigProperty Key, int32 DefaultValue)
{
    if (!ConfigValues.Contains(Key))
    {
        return DefaultValue;        
    }
    
    const FString StringValue = GetStringValue(Key);
    if (!StringValue.IsEmpty())
    {
        return FCString::Atoi(*StringValue);
    }

    return DefaultValue;
}

bool ULiveConfigSystem::GetBoolValue(FLiveConfigProperty Key, bool bDefault)
{
    if (!ConfigValues.Contains(Key))
    {
        return bDefault;        
    }
    
    const FString StringValue = GetStringValue(Key);
    if (StringValue.Contains("true"))
    {
        return true;
    }
    
    return false;
}