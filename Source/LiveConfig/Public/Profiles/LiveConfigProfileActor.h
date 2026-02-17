// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LiveConfigProfile.h"
#include "LiveConfigProfileActor.generated.h"

/**
 * Actor to replicate the active Live Config profile.
 * Spawned by ULiveConfigProfileSubsystem.
 */
UCLASS(NotBlueprintable)
class LIVECONFIG_API ALiveConfigProfileActor : public AActor
{
    GENERATED_BODY()

public:
    ALiveConfigProfileActor();
    
    // AActor
    virtual void BeginPlay() override;
    // ~AActor 
    
    UFUNCTION(Meta = (WorldContext = WorldContext))
    static ALiveConfigProfileActor* Get(const UObject* WorldContext);

    /** 
     * Server-side: Set the active profile by name. Loads from disk if available. 
     * 
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Live Config|Profiles")
    void ServerSetActiveProfile(FName ProfileName);

    /**
     * Server-side: Set the active profile with full data. Replicates to all clients. 
     * RPC to set profile data 
     */
    UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Live Config|Profiles")
    void ServerSetActiveProfileData(const FLiveConfigProfile& Profile);

protected:
    UPROPERTY(ReplicatedUsing = OnRep_ActiveProfile)
    FLiveConfigProfile ReplicatedActiveProfile;

    UFUNCTION()
    void OnRep_ActiveProfile();

    void ApplyProfileToSystem();
};
