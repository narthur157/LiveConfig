// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#include "LiveConfigUserSettings.h"

FString ULiveConfigUserSettings::GetSourcePath()
{
	const ULiveConfigUserSettings* Settings = GetDefault<ULiveConfigUserSettings>();
	switch (Settings->ExternalSourceOverride)
	{
	case ELiveConfigSourceType::None:
		return {};
	case ELiveConfigSourceType::HttpCsv:
		return Settings->RemoveCsvUrl;
	case ELiveConfigSourceType::LocalCsv:
		return FPaths::ConvertRelativePathToFull(Settings->LocalCSVPath.FilePath);
	}
	
	checkNoEntry();
	return {};
}
