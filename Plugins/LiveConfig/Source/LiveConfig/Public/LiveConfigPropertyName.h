#pragma once

#include "CoreMinimal.h"
#include "LiveConfigPropertyName.generated.h"

/**
 * A type-safe wrapper for Live Config row names, similar to FGameplayTag.
 * Provides better editor integration and type safety.
 */
USTRUCT(BlueprintType, Meta = (HasNativeMake = "/Script/LiveConfig.LiveConfigLibrary.MakeLiteralLiveConfigProperty", HasNativeBreak = "/Script/LiveConfig.LiveConfigLibrary.GetPropertyName", DisableSplitPin))
struct LIVECONFIG_API FLiveConfigProperty
{
	GENERATED_BODY()

	FLiveConfigProperty() : PropertyName(NAME_None) {}
	FLiveConfigProperty(FName InPropertyName) : PropertyName(InPropertyName) {}
	FLiveConfigProperty(const FString& InPropertyStr) : PropertyName(*InPropertyStr) {}
    
	/** Get the underlying FName */
	FName GetName() const { return PropertyName; }

	/** Get the row name as a string */
	FString ToString() const { return PropertyName.ToString(); }

	/** Check if this is a valid row name */
	bool IsValid() const { return PropertyName != NAME_None; }

	/** Comparison operators */
	bool operator==(const FLiveConfigProperty& Other) const { return PropertyName == Other.PropertyName; }
	bool operator!=(const FLiveConfigProperty& Other) const { return PropertyName != Other.PropertyName; }

	bool ExportTextItem(FString& ValueStr, FLiveConfigProperty const& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const;
	bool ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText);

	FORCEINLINE friend uint32 GetTypeHash(const FLiveConfigProperty& Prop)
	{
		return GetTypeHash(Prop.GetName());
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Categories = "LiveConfig"))
	FName PropertyName;
};

template<>
struct TStructOpsTypeTraits<FLiveConfigProperty> : public TStructOpsTypeTraitsBase2<FLiveConfigProperty>
{
	enum
	{
		WithExportTextItem = true,
		WithImportTextItem = true,
	};
};

