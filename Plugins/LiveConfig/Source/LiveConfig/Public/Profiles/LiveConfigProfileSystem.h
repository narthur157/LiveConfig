#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "LiveConfigProfile.h"
#include "LiveConfigProfileSystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLiveConfigProfile, Log, All);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLiveConfigProfileChanged, const FLiveConfigProfile&);

UCLASS(BlueprintType)
class LIVECONFIG_API ULiveConfigProfileSystem : public UEngineSubsystem
{
    GENERATED_BODY()

public:
    static ULiveConfigProfileSystem* Get();

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "Live Config|Profiles")
    void SaveProfile(const FLiveConfigProfile& Profile);

    UFUNCTION(BlueprintCallable, Category = "Live Config|Profiles")
    bool LoadProfile(FName ProfileName, FLiveConfigProfile& OutProfile);

    UFUNCTION(BlueprintCallable, Category = "Live Config|Profiles")
    void DeleteProfile(FName ProfileName);

    UFUNCTION(BlueprintCallable, Category = "Live Config|Profiles")
    TArray<FName> GetAllProfileNames() const;

    UFUNCTION(BlueprintCallable, Category = "Live Config|Profiles")
    void SetActiveProfile(FName ProfileName);

    /** 
     * Set the active profile 
     * Replicates it if possible
     */
    void SetActiveProfileData(const FLiveConfigProfile& Profile);
    
    void SetActiveProfileData_NoReplication(const FLiveConfigProfile& Profile);

    UFUNCTION(BlueprintCallable, Category = "Live Config|Profiles")
    const FLiveConfigProfile& GetActiveProfile() const { return ActiveProfile; }

    FOnLiveConfigProfileChanged OnProfileChanged;

    static FString GetProfilesDirectory();
    static FString GetProfilePath(FName ProfileName);

private:
    void PopulateAutoCompleteEntries(TArray<struct FAutoCompleteCommand>& Entries);

    FLiveConfigProfile ActiveProfile;
};
