#pragma once

#include "CoreMinimal.h"
#include "LiveConfigPropertyName.h"
#include "Profiles/LiveConfigProfile.h"
#include "LiveConfigTypes.generated.h"

LIVECONFIG_API DECLARE_LOG_CATEGORY_EXTERN(LogLiveConfig, Log, All);

UENUM(BlueprintType)
enum class ELiveConfigPropertyChangeType : uint8
{
	Name,
	Description,
	Type,
	Value,
	Tags
};

UENUM(BlueprintType)
enum class ELiveConfigPropertyType : uint8
{
	String,
	Int,
	Float,
	Bool
};

USTRUCT(BlueprintType)
struct LIVECONFIG_API FLiveConfigPropertyDefinition
{
	GENERATED_BODY()

	UPROPERTY(Config, BlueprintReadWrite, EditAnywhere, Category = "Property")
	FLiveConfigProperty PropertyName;

	UPROPERTY(Config, BlueprintReadWrite, EditAnywhere, Category = "Property")
	FString Description;

	UPROPERTY(Config, BlueprintReadWrite, EditAnywhere, Category = "Property")
	ELiveConfigPropertyType PropertyType = ELiveConfigPropertyType::String;

	UPROPERTY(Config, BlueprintReadWrite, EditAnywhere, Category = "Property")
	TArray<FName> Tags;

	UPROPERTY(Config, BlueprintReadWrite, EditAnywhere, Category = "Property")
	FString Value;
	
	bool IsValid() const;
};

/**
 * Low-level cache of the actual config data with the specific primitive types
 * Used to cache config values for faster access
 */
USTRUCT()
struct FLiveConfigCache
{
	GENERATED_BODY()
	
	UPROPERTY()
	TMap<FLiveConfigProperty, float> FloatValues;
	UPROPERTY()
	TMap<FLiveConfigProperty, int32> IntValues;
	UPROPERTY()
	TMap<FLiveConfigProperty, FString> StringValues;
	UPROPERTY()
	TMap<FLiveConfigProperty, bool> BoolValues;
	
	void Reset();
	void SetValue(const FLiveConfigPropertyDefinition& InValue);
		
	template<typename T>
	T GetValue(const FLiveConfigProperty& Property) const;

	static void BuildConfig(const TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition>& PropertyDefinitions, 
		const FLiveConfigProfile& EnvironmentProfile, const FLiveConfigProfile& ActiveProfile, FLiveConfigCache& OutCache);
};

template<>
inline float FLiveConfigCache::GetValue<float>(const FLiveConfigProperty& Property) const
{
	if (const float* Value = FloatValues.Find(Property))
	{
		return *Value;
	}
	
	UE_LOG(LogLiveConfig, Warning, TEXT("Failed to find float value for property: %s"), *Property.ToString());
	return 0.0f;
}

template<>
inline int32 FLiveConfigCache::GetValue<int32>(const FLiveConfigProperty& Property) const
{
	if (const int32* Value = IntValues.Find(Property))
	{
		return *Value;
	}
	
	UE_LOG(LogLiveConfig, Warning, TEXT("Failed to find int32 value for property: %s"), *Property.ToString());
	return 0;
}

template<>
inline FString FLiveConfigCache::GetValue<FString>(const FLiveConfigProperty& Property) const
{
	if (const FString* Value = StringValues.Find(Property))
	{
		return *Value;
	}
	
	UE_LOG(LogLiveConfig, Warning, TEXT("Failed to find string value for property: %s"), *Property.ToString());
	return FString();
}

template<>
inline bool FLiveConfigCache::GetValue<bool>(const FLiveConfigProperty& Property) const
{
	if (const bool* Value = BoolValues.Find(Property))
	{
		return *Value;
	}
	
	UE_LOG(LogLiveConfig, Warning, TEXT("Failed to find bool value for property: %s"), *Property.ToString());
	return false;
}


