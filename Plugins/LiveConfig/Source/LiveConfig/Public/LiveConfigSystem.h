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
    FOnLiveConfigPropertiesUpdated OnPropertiesUpdated;
	
	FSimpleMulticastDelegate OnTagsChanged;
	
    /**
     * note - if using this very early during startup (eg from another UEngineSubsystem::Initialize) consider
     * using GEngine->GetEngineSubsystem<ULiveConfigSystem>() to check
     * @return Checked reference to the live config engine subsystem
     */
    static ULiveConfigSystem& Get();

    // USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    // ~USubsystem

    /**
     * @param PropertyDefinition Property to be added or updated. Will be saved to file
     */
    void SaveProperty(const FLiveConfigPropertyDefinition& PropertyDefinition);

    /**
     * Renames an existing property and its associated files on disk.
     * If the property is a struct, it will also rename all its members.
     * @param OldName Current name of the property
     * @param NewName New name for the property
     * @param bCreateRedirector Whether to create a redirector from the old name to the new name
     */
    void RenameProperty(FLiveConfigProperty OldName, FLiveConfigProperty NewName, bool bCreateRedirector = false);

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
     * Look up values for a struct's properties using the property name as a prefix for its members.
     * Each UProperty in the struct will be looked up as StructProperty.PropertyName.
     */
    template<typename T>
    T GetLiveConfigStruct(FLiveConfigProperty StructProperty) const
    {
        T OutStruct;
        UScriptStruct* Struct = TBaseStructure<T>::Get();
        if (!Struct)
        {
            Struct = T::StaticStruct();
        }
        GetLiveConfigStruct_Internal(Struct, &OutStruct, StructProperty);
        return OutStruct;
    }

    /**
     * @tparam T Must be a numeric type, bool, or string. Does NOT support structs
     * @param Property Property key to lookup - may be implicitly converted
     */
    template<typename T>
	T GetLiveConfigValue(FLiveConfigProperty Property) const
    {
        RedirectPropertyName(Property);
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

    /** Get all available row names (public for editor access) */
	UFUNCTION(BlueprintCallable, Category = "Live Config")
	const TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition>& GetAllProperties() const;

    static bool DoesPropertyNameExist(FLiveConfigProperty PropertyName);

    /** Redirects a property name based on the redirectors map */
    void RedirectPropertyName(FLiveConfigProperty& Property) const;

    /**
     * Gets all properties that are not referenced in any assets or config files
     * @param OutUnusedProperties Array to fill with unused property names
     */
    void GetUnusedProperties(TArray<FName>& OutUnusedProperties) const;

    /**
     * Gets all redirects that point to properties that no longer exist
     * @param OutUnusedRedirects Array to fill with redirect keys (old names) that point to non-existent properties and are not referenced
     */
    void GetUnusedRedirects(TArray<FName>& OutUnusedRedirects) const;

    /**
     * Checks if a property name (typically an old redirect name) is still referenced in any blueprint assets
     * Note: This only checks assets, not C++ code. User should search code separately.
     * @param PropertyName The property name to search for
     * @param OutReferencingAssets Optional array to fill with asset paths that reference this property
     * @return True if the property is found in any assets
     */
    bool IsPropertyReferencedInAssets(FName PropertyName, TArray<FString>* OutReferencingAssets = nullptr) const;

    /**
     * Removes a specific redirect from the map and saves the config
     * @param OldPropertyName The old property name to remove from redirects
     */
    void RemoveRedirect(FName OldPropertyName);

    /**
     * Removes all redirects that point to properties that no longer exist
     * @return Number of redirects removed
     */
    int32 CleanupUnusedRedirects();

    /**
     * Gets all names related to a property, including redirects (old names)
     * @param PropertyName The name to find related names for
     * @param OutRelatedNames Array to fill with related names
     */
    void GetRelatedPropertyNames(FName PropertyName, TArray<FName>& OutRelatedNames) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "General")
	TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition> PropertyDefinitions;
	
	UPROPERTY(Config, VisibleAnywhere, BlueprintReadOnly, Category = "General")
	TArray<FName> PropertyTags;
	
	void HandleTagsChanged();
	

    UPROPERTY(Config)
    TMap<FName, FName> PropertyRedirects;


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
