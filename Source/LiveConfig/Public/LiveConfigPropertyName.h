// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License
#pragma once

#include "CoreMinimal.h"
#include "LiveConfigPropertyName.generated.h"

/**
 * A type-safe wrapper for Live Config row names, similar to FGameplayTag.
 * Provides better editor integration and type safety.
 */
USTRUCT(BlueprintType, Meta = (HasNativeMake = "/Script/LiveConfig.LiveConfigLib.MakeLiteralLiveConfigProperty", HasNativeBreak = "/Script/LiveConfig.LiveConfigLib.GetPropertyName", DisableSplitPin))
struct LIVECONFIG_API FLiveConfigProperty
{
	GENERATED_BODY()

	FLiveConfigProperty() : PropertyName(NAME_None) {}
	FLiveConfigProperty(FName InPropertyName) : PropertyName(InPropertyName) {}
	FLiveConfigProperty(FName InPropertyName, bool bRedirect);
	// Note this version will *not* handle redirects
	FLiveConfigProperty(const FString& InPropertyStr) : PropertyName(*InPropertyStr) {}
	FLiveConfigProperty(const FString& InPropertyStr, bool bRedirect);
	FLiveConfigProperty(const TCHAR* InPropertyStr) : PropertyName(InPropertyStr) {}
    
	/** Get the underlying FName */
	FName GetName() const { return PropertyName; }

	/** Get the row name as a string */
	FString ToString() const { return PropertyName.ToString(); }

	/** Check if this is a valid row name */
	bool IsValid() const { return PropertyName != NAME_None; }

	/** Comparison operators */
	bool operator==(const FLiveConfigProperty& Other) const { return PropertyName == Other.PropertyName; }
	bool operator!=(const FLiveConfigProperty& Other) const { return PropertyName != Other.PropertyName; }

	/**
	 * Creates a redirected Live Config property from a name.
	 * This is the preferred way to create properties in C++ to ensure redirects are handled.
	 */
	static FLiveConfigProperty Request(FName InPropertyName);

	bool ExportTextItem(FString& ValueStr, FLiveConfigProperty const& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const;
	bool ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText);

	FORCEINLINE friend uint32 GetTypeHash(const FLiveConfigProperty& Prop)
	{
		return GetTypeHash(Prop.GetName());
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Categories = "LiveConfig"))
	FName PropertyName;

	/** Helper to sync name in editor */
	void PostSerialize(const FArchive& Ar);
};

/** Literal operator for Live Config properties, which automatically handles redirects. */
FORCEINLINE FLiveConfigProperty operator""_LC(const char* Str,size_t)
{
	return FLiveConfigProperty::Request(FName(Str));
}

template<>
struct TStructOpsTypeTraits<FLiveConfigProperty> : public TStructOpsTypeTraitsBase2<FLiveConfigProperty>
{
	enum
	{
		WithExportTextItem = true,
		WithImportTextItem = true,
		WithPostSerialize = true,
	};
};

