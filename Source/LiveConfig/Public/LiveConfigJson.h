// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

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
	
	// USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// ~USubsystem
	
	void LoadJsonFromFiles();

	UFUNCTION(Exec)
	void VerifyJsonIntegrity();

	void SavePropertyToFile(const FLiveConfigPropertyDefinition& PropertyDefinition);
	void DeletePropertyFile(FName PropertyName);
	void RenamePropertyOnDisk(FName OldPropertyName, FName NewPropertyName);

	static FString GetPropertyPath(FName PropertyName);
	static FString GetLiveConfigDirectory();
	
	void FlushPendingSaves();

	// for testing
	bool bDisableFileOperations = false;
private:
	void LoadJsonFromDirectory(const FString& Dir);
	void LoadJsonFromFile(const FString& Path, const FString& FileName);
	
	void QueueSave();
	bool OnTick(float DeltaTime);

	TMap<FName, FLiveConfigPropertyDefinition> QueuedSaves;
	TSet<FName> QueuedDeletions;
	FTSTicker::FDelegateHandle TickHandle;
};
