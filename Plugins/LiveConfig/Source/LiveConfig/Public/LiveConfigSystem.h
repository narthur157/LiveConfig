#pragma once

#include "Subsystems/EngineSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "CoreMinimal.h"
#include "LiveConfigTypes.h"
#include "LiveConfigPropertyName.h"
#include "Profiles/LiveConfigProfile.h"
#include "LiveConfigSystem.generated.h"
DECLARE_MULTICAST_DELEGATE(FOnLiveConfigPropertiesUpdated);

namespace LiveConfigTags
{
	const LIVECONFIG_API extern FName FromCurveTable;
}

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
     * @return The found value or the default from property definition.
     */
    UFUNCTION(BlueprintCallable, Category = "Live Config")
    FString GetStringValue(FLiveConfigProperty Key);

    /** Gets a configuration value as a float. */
    UFUNCTION(BlueprintCallable, Category = "Live Config")
    float GetFloatValue(FLiveConfigProperty Key);

    /** Gets a configuration value as an integer. */
    UFUNCTION(BlueprintCallable, Category = "Live Config")
    int32 GetIntValue(FLiveConfigProperty Key);

    /** Gets a configuration value as a boolean. */
    UFUNCTION(BlueprintCallable, Category = "Live Config")
    bool GetBoolValue(FLiveConfigProperty Key);

    /** Returns true if the data has been successfully downloaded and parsed. */
    UFUNCTION(BlueprintCallable, Category = "Live Config")
    bool IsDataReady() const { return bIsDataReady; }

    /**
     * Download the config
     */
    UFUNCTION(BlueprintCallable)
    void CheckUpdate();
    
    void DownloadConfig();

    /** Refresh properties from editor settings */
    void RefreshFromSettings();
    void RefreshFromSettings(const FLiveConfigProfile& Profile);

    FOnLiveConfigPropertiesUpdated OnPropertiesUpdated;

    /** Get all available row names (public for editor access) */
    UFUNCTION(BlueprintCallable, Category = "Live Config")
    TArray<FLiveConfigProperty> GetAllProperties() const;

    bool DoesPropertyNameExist(FLiveConfigProperty PropertyName) const;


private:
    FString SheetUrl;
    double TimeLoadStarted = -9999;
    float RateLimitSeconds = 5;
    FTimerHandle PollingTimer;
    FTSTicker::FDelegateHandle TimeoutTimer;
    TSharedPtr<IHttpRequest> CurrentRequest;
	
	UPROPERTY()
	FLiveConfigCache Cache;
    
	// Environment config is treated as a layer below more specific profiles
	FLiveConfigProfile EnvironmentOverrides;
    
    float TimeoutDuration;
    /** Callback function for when the HTTP request completes. */
    void OnSheetDownloadComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    void OnTravel(UWorld* World, FWorldInitializationValues WorldInitializationValues);
    void OnStartGameInstance(UGameInstance* GameInstance);

    /**
     * Called to rebuild the lookup tables whenever a layer or the base config changes
     */
	UFUNCTION()
    void BuildCache();

    /** Flag to indicate if the download and parsing were successful. */
    bool bIsDataReady = false;
};
