// Copyright 2026 Nicholas Arthur - All Rights Reserved

#pragma once

#include "CoreMinimal.h"

#include "LiveConfigTestActor.generated.h"

USTRUCT(BlueprintType)
struct FLiveConfigSampleStruct
{
	GENERATED_BODY()
	
	UPROPERTY()
	bool bSampleBool = false;
	
	// Only UPROPERTY marked fields are included in Live Config
	bool bSampleNonUpropertyBool = false;
	
	UPROPERTY()
	FString SampleString;
	
	UPROPERTY(BlueprintReadWrite)
	FString BlueprintSampleString;
};

USTRUCT(BlueprintType)
struct FLiveConfigSampleNestedStruct
{
	GENERATED_BODY()
	
	UPROPERTY()
	FLiveConfigSampleStruct SampleStructA;
	
	UPROPERTY()
	FLiveConfigSampleStruct SampleStructB;
	
	UPROPERTY()
	FVector Vector;
};

UCLASS()
class LIVECONFIGSAMPLE_API ALiveConfigTestActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ALiveConfigTestActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
};
