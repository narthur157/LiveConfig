// All rights reserved - Genpop

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "LiveConfigSystem.h"
#include "LiveConfigJson.generated.h"

/**
 * Manager of the json state
 */
UCLASS(BlueprintType)
class LIVECONFIG_API ULiveConfigJsonSystem : public UEngineSubsystem
{
	GENERATED_BODY()
public:
	static ULiveConfigJsonSystem* Get();
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	void LoadJsonFromFiles();

	UFUNCTION(BlueprintCallable)
	void SaveJsonToFiles();

	UFUNCTION(Exec)
	void VerifyJsonIntegrity();

	void SavePropertyToFile(const FLiveConfigPropertyDefinition& PropertyDefinition);
	void DeletePropertyFile(FName PropertyName);

	void CheckoutProperties(const TArray<FName>& PropertyNames);

	static FString GetPropertyPath(FName PropertyName);
	static FString GetLiveConfigDirectory();
private:
	void LoadJsonFromDirectory(const FString& Dir);
	void LoadJsonFromFile(const FString& Path, const FString& FileName);
	
	TArray<FString> PendingCheckoutFiles;
};
