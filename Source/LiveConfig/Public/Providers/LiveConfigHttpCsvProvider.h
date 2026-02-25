// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#pragma once

#include "CoreMinimal.h"
#include "LiveConfigTypes.h"
#include "Interfaces/IHttpRequest.h"
#include "LiveConfigHttpCsvProvider.generated.h"

/**
 * Default provider that fetches overrides from a CSV URL via HTTP.
 */
UCLASS()
class LIVECONFIG_API ULiveConfigHttpCsvProvider : public ULiveConfigRemoteOverrideProvider
{
	GENERATED_BODY()

public:
	virtual void FetchOverrides(const FOnRemoteOverridesFetched& OnComplete) override;

private:
	void OnDownloadComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, FOnRemoteOverridesFetched OnComplete);

	TSharedPtr<IHttpRequest> CurrentRequest;
};
