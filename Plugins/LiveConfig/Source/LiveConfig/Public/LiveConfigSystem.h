#pragma once

#include "UObject/UnrealType.h"
#include "Subsystems/EngineSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "CoreMinimal.h"
#include "LiveConfigTypes.h"
#include "LiveConfigPropertyName.h"
#include "Profiles/LiveConfigProfile.h"
#include "ConsoleSettings.h"
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
    FString GetStringValue(FLiveConfigProperty Key) const;

    /** Gets a configuration value as a float. */
    float GetFloatValue(FLiveConfigProperty Key) const;

    /** Gets a configuration value as an integer. */
    int32 GetIntValue(FLiveConfigProperty Key) const;

    /** Gets a configuration value as a boolean. */
    bool GetBoolValue(FLiveConfigProperty Key) const;

    /**
     * Look up values for a struct's properties using a prefix.
     * Each UProperty in the struct will be looked up as Prefix.PropertyName.
     */
    template<typename T>
    T GetStructValue(FLiveConfigProperty Prefix) const
    {
        T OutStruct;
        UScriptStruct* Struct = TBaseStructure<T>::Get();
        if (!Struct)
        {
            Struct = T::StaticStruct();
        }
        GetLiveConfigStruct_Internal(Struct, &OutStruct, Prefix);
        return OutStruct;
    }
	
	template<typename T>
	T GetLiveConfigValue(FLiveConfigProperty Property) const
    {
	    const FLiveConfigPropertyDefinition* Def = PropertyDefinitions.Find(Property);
    	if (!Def)
    	{
    		return {};
    	}
    
    	if (Def->PropertyType == ELiveConfigPropertyType::Struct)
    	{
    		return GetStructValue<T>(Property);
    	}
    	
		return Cache.GetValue<T>(Property);
    }

    void GetLiveConfigStruct_Internal(class UScriptStruct* Struct, void* OutStructPtr, FLiveConfigProperty Prefix) const;

    /** Returns true if the data has been successfully downloaded and parsed. */
    UFUNCTION(BlueprintCallable, Category = "Live Config")
    bool IsDataReady() const { return bIsDataReady; }

    void DownloadConfig();

    /** Refresh properties from editor settings */
    void RebuildConfigCache();
    void RebuildConfigCache(const FLiveConfigProfile& Profile);

    FOnLiveConfigPropertiesUpdated OnPropertiesUpdated;

    /** Get all available row names (public for editor access) */
	UFUNCTION(BlueprintCallable, Category = "Live Config")
	const TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition>& GetAllProperties() const;

    static bool DoesPropertyNameExist(FLiveConfigProperty PropertyName);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
	TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition> PropertyDefinitions;


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
	
	void PopulateAutoCompleteEntries(TArray<FAutoCompleteCommand>& AutoCompleteCommands);

    /** Flag to indicate if the download and parsing were successful. */
    bool bIsDataReady = false;
};
