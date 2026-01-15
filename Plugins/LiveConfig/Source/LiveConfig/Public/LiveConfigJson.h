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

	UFUNCTION(Exec)
	void VerifyJsonIntegrity();

	void SavePropertyToFile(const FLiveConfigPropertyDefinition& PropertyDefinition);
	void DeletePropertyFile(FName PropertyName);

	static FString GetPropertyPath(FName PropertyName);
	static FString GetLiveConfigDirectory();
private:
	void LoadJsonFromDirectory(const FString& Dir);
	void LoadJsonFromFile(const FString& Path, const FString& FileName);
	
	void QueueSave();
	bool OnTick(float DeltaTime);

	TMap<FName, FLiveConfigPropertyDefinition> QueuedSaves;
	TSet<FName> QueuedDeletions;
	FTSTicker::FDelegateHandle TickHandle;
};
