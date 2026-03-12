// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#include "LiveConfigSettings.h"

FString ULiveConfigSettings::GetSourcePath()
{
	const ULiveConfigSettings* Settings = GetDefault<ULiveConfigSettings>();
	switch (Settings->DefaultExternalSource)
	{
	case ELiveConfigSourceType::None:
		return {};
	case ELiveConfigSourceType::HttpCsv:
		return Settings->RemoteCsvUrl;
	case ELiveConfigSourceType::LocalCsv:
		return FPaths::ProjectSavedDir() / "LiveConfigOverrides.csv";
	}
	
	checkNoEntry();
	return {};
}
