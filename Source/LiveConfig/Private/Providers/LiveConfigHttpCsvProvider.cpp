// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#include "Providers/LiveConfigHttpCsvProvider.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "LiveConfigSettings.h"
#include "LiveConfigLib.h"

void ULiveConfigHttpCsvProvider::FetchOverrides(const FOnRemoteOverridesFetched& OnComplete)
{
	if (CurrentRequest && !CurrentRequest->GetResponse())
	{
		return;
	}

	const ULiveConfigSettings* Settings = GetDefault<ULiveConfigSettings>();
	if (Settings->RemoteOverrideCSVUrl.IsEmpty())
	{
		UE_LOG(LogLiveConfig, Log, TEXT("LiveConfigHttpCsvProvider: RemoteOverrideCSVUrl is empty."));
		OnComplete.ExecuteIfBound(FLiveConfigProfile());
		return;
	}

	FHttpModule& HttpModule = FHttpModule::Get();
	CurrentRequest = HttpModule.CreateRequest();

	CurrentRequest->OnProcessRequestComplete().BindUObject(this, &ULiveConfigHttpCsvProvider::OnDownloadComplete, OnComplete);
	CurrentRequest->SetURL(Settings->RemoteOverrideCSVUrl);
	CurrentRequest->SetVerb(TEXT("GET"));
	CurrentRequest->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
	CurrentRequest->SetHeader(TEXT("Content-Type"), TEXT("text/csv"));
	
	CurrentRequest->ProcessRequest();
}

void ULiveConfigHttpCsvProvider::OnDownloadComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, FOnRemoteOverridesFetched OnComplete)
{
	FLiveConfigProfile Profile;
	if (bWasSuccessful && Response.IsValid())
	{
		const FString CsvContent = Response->GetContentAsString();
		Profile = ULiveConfigLib::ParseOverridesFromCsv(CsvContent);
	}
	else
	{
		UE_LOG(LogLiveConfig, Error, TEXT("LiveConfigHttpCsvProvider: Download failed."));
	}

	OnComplete.ExecuteIfBound(Profile);
}
