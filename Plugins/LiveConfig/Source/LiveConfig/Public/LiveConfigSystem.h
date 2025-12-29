#pragma once

#include "Subsystems/EngineSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "CoreMinimal.h"
#include "LiveConfigPropertyName.h"
#include "LiveConfigSystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLiveConfig, Log, Log);

USTRUCT(BlueprintType)
struct FLiveConfigPropertyDefinition
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FLiveConfigRowName PropertyName;

    UPROPERTY(BlueprintReadWrite)
    FString Description;

    UPROPERTY(BlueprintReadWrite)
    TArray<FName> Tags;
};

/**
 * Property *values*
 */
USTRUCT(BlueprintType)
struct FLiveConfigProperty
{
    GENERATED_BODY()

    FString RawValue;
    FString Description;
    
};


/**
 * A subsystem to download configuration data from a public Google Sheet URL (published as CSV).
 * It populates a TMap which can be queried for string, float, or int values.
 */
UCLASS(BlueprintType, Config=Game, DefaultConfig)
class LIVECONFIG_API ULiveConfigSystem : public UEngineSubsystem
{
    GENERATED_BODY()

public:
    static ULiveConfigSystem* Get();
    
    // USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    // ~USubsystem

    /**
     * Gets a configuration value as a string.
     * @param Key The CVar-like name to look up.
     * @param DefaultValue The value to return if the key is not found or data isn't loaded.
     * @return The found value or the default.
     */
    UFUNCTION(BlueprintCallable, Category = "Live Config")
    FString GetStringValue(FName Key, const FString& DefaultValue = "");

    /** Gets a configuration value as a float. */
    UFUNCTION(BlueprintCallable, Category = "Live Config")
    float GetFloatValue(FName Key, float DefaultValue = 0.0f);

    /** Gets a configuration value as a float. */
    UFUNCTION(BlueprintCallable, Category = "Live Config")
    bool GetBoolValue(FName Key, bool bDefault = false);

    /** Returns true if the data has been successfully downloaded and parsed. */
    UFUNCTION(BlueprintCallable, Category = "Live Config")
    bool IsDataReady() const { return bIsDataReady; }

    /**
     * Download the config
     */
    UFUNCTION(BlueprintCallable)
    void CheckUpdate();
    
    void DownloadConfig();

    /** Get all available row names (public for editor access) */
    UFUNCTION(BlueprintCallable, Category = "Live Config")
    TArray<FLiveConfigRowName> GetAllRowNames() const;

    bool DoesPropertyNameExist(FName PropertyName) const;


private:
    FString SheetUrl;
    double TimeLoadStarted = -9999;
    float RateLimitSeconds = 5;
    FTimerHandle PollingTimer;
    FTSTicker::FDelegateHandle TimeoutTimer;
    TSharedPtr<IHttpRequest> CurrentRequest;
    
    float TimeoutDuration;
    /** Callback function for when the HTTP request completes. */
    void OnSheetDownloadComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    /** The main storage for our key-value pairs. */
    UPROPERTY(VisibleAnywhere)
    TMap<FLiveConfigRowName, FLiveConfigProperty> ConfigValues;
    
    void OnTravel(UWorld* World, FWorldInitializationValues WorldInitializationValues);
    void OnStartGameInstance(UGameInstance* GameInstance);

    /** Flag to indicate if the download and parsing were successful. */
    bool bIsDataReady = false;
};
