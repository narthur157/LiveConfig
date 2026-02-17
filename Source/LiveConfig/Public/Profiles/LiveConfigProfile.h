// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License
 
#pragma once

#include "CoreMinimal.h"
#include "LiveConfigPropertyName.h"
#include "LiveConfigProfile.generated.h"

USTRUCT(BlueprintType)
struct FLiveConfigOverride
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Live Config")
    FLiveConfigProperty Property;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Live Config")
    FString Value;
};

USTRUCT(BlueprintType)
struct FLiveConfigProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Live Config")
    FName ProfileName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Live Config", NotReplicated, SkipSerialization)
    TMap<FLiveConfigProperty, FString> Overrides;

    bool IsValid() const { return ProfileName != NAME_None; }
    
    /**
     * Update profile so that its properties are redirected accordingly
     * This is used automatically whenever profiles are applied, so it should not need to be used in general 
    */ 
    void Redirect();
    
    // May be able to handle this behavior via a net serializer rather than forcing us to do a pre/post call
    void PrepareForReplication()
    {
        ReplicatedOverrides.Empty();
        for (const auto& Pair : Overrides)
        {
            ReplicatedOverrides.Add({Pair.Key, Pair.Value});
        }
    }

    void OnAfterReplication()
    {
        Overrides.Empty();
        for (const auto& Override : ReplicatedOverrides)
        {
            Overrides.Add(Override.Property, Override.Value);
        }
    }

    bool operator==(const FLiveConfigProfile& OtherProfile) const;

private:
    /** For replication as TMap doesn't replicate */
    UPROPERTY(meta = (SkipSerialization))
    TArray<FLiveConfigOverride> ReplicatedOverrides;

};

inline bool FLiveConfigProfile::operator==(const FLiveConfigProfile& OtherProfile) const
{
    if (OtherProfile.Overrides.Num() != Overrides.Num())
    {
        return false;
    }
    
    for (auto OverridePair : Overrides)
    {
        if (!OtherProfile.Overrides.Contains(OverridePair.Key))
        {
            return false;
        }
        
        if (OtherProfile.Overrides[OverridePair.Key] != OverridePair.Value)
        {
            return false;
        }
    }
    
    return true;
}
