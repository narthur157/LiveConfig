#pragma once

#include "CoreMinimal.h"
#include "LiveConfigPropertyName.generated.h"

/**
 * A type-safe wrapper for Live Config row names, similar to FGameplayTag.
 * Provides better editor integration and type safety.
 */
USTRUCT(BlueprintType, Meta = (HasNativeMake = "/Script/LiveConfig.LiveConfigLibrary.MakeLiteralLiveConfigRowName", HasNativeBreak = "/Script/LiveConfig.LiveConfigLibrary.GetRowName", DisableSplitPin))
struct LIVECONFIG_API FLiveConfigRowName
{
	GENERATED_BODY()

	FLiveConfigRowName() : RowName(NAME_None) {}
	FLiveConfigRowName(FName InRowName) : RowName(InRowName) {}
	FLiveConfigRowName(const FString& InRowName) : RowName(*InRowName) {}
    
	/** Get the underlying FName */
	FName GetRowName() const { return RowName; }

	/** Get the row name as a string */
	FString ToString() const { return RowName.ToString(); }

	/** Check if this is a valid row name */
	bool IsValid() const { return RowName != NAME_None; }

	/** Comparison operators */
	bool operator==(const FLiveConfigRowName& Other) const { return RowName == Other.RowName; }
	bool operator!=(const FLiveConfigRowName& Other) const { return RowName != Other.RowName; }

	FORCEINLINE friend uint32 GetTypeHash(const FLiveConfigRowName& Prop)
	{
		return GetTypeHash(Prop.GetRowName());
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Categories = "LiveConfig"))
	FName RowName;
};

