#pragma once

#include "CoreMinimal.h"
#include "LiveconfigStructTest.generated.h"

USTRUCT(BlueprintType)
struct FLiveConfigTestStruct
{
	GENERATED_BODY()

	UPROPERTY()
	FString SomeString;

	UPROPERTY()
	int32 SomeInt = 0;

	UPROPERTY()
	float SomeFloat = 0.0f;

	UPROPERTY()
	bool SomeBool = false;
};
