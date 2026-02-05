#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "LiveConfigExportCommandlet.generated.h"

UCLASS()
class ULiveConfigExportCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	ULiveConfigExportCommandlet();

	virtual int32 Main(const FString& Params) override;

private:
	FString EscapeCSV(const FString& InString);
};
