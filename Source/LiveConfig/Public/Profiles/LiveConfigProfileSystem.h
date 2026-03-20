// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

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

    FOnLiveConfigProfileChanged OnProfileChanged;

    // USubsystem
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    // ~USubsystem

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

    static FString GetProfilesDirectory();
    static FString GetProfilePath(FName ProfileName);

private:
    FLiveConfigProfile ActiveProfile;
};
