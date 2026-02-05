#include "LiveConfigExportCommandlet.h"
#include "LiveConfigSystem.h"
#include "LiveConfigJson.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "LiveConfigTypes.h"

ULiveConfigExportCommandlet::ULiveConfigExportCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 ULiveConfigExportCommandlet::Main(const FString& Params)
{
	UE_LOG(LogLiveConfig, Log, TEXT("Running LiveConfigExportCommandlet..."));

	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> ParamMap;
	ParseCommandLine(*Params, Tokens, Switches, ParamMap);

	FString ExportPath;
	if (ParamMap.Contains(TEXT("Output")))
	{
		ExportPath = ParamMap[TEXT("Output")];
	}
	else
	{
		ExportPath = FPaths::ProjectSavedDir() / TEXT("LiveConfigExport.csv");
	}

	// We need to make sure the LiveConfigSystem is initialized and has loaded all properties
	ULiveConfigSystem& System = ULiveConfigSystem::Get();
	
	// Manually trigger load if needed (though Initialize should have done it if we are in an environment where subsystems are up)
	if (ULiveConfigJsonSystem* JsonSystem = ULiveConfigJsonSystem::Get())
	{
		JsonSystem->LoadJsonFromFiles();
	}
	
	const TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition>& AllProperties = System.GetAllProperties();

	if (AllProperties.Num() == 0)
	{
		UE_LOG(LogLiveConfig, Warning, TEXT("No Live Config properties found to export."));
		return 0;
	}

	FString CsvContent = TEXT("Name,Value,Type,Description,Tags\n");

	for (auto& Pair : AllProperties)
	{
		const FLiveConfigPropertyDefinition& Def = Pair.Value;

		FString PropertyTypeStr;
		switch (Def.PropertyType)
		{
			case ELiveConfigPropertyType::String: PropertyTypeStr = TEXT("String"); break;
			case ELiveConfigPropertyType::Int:    PropertyTypeStr = TEXT("Int"); break;
			case ELiveConfigPropertyType::Float:  PropertyTypeStr = TEXT("Float"); break;
			case ELiveConfigPropertyType::Bool:   PropertyTypeStr = TEXT("Bool"); break;
			case ELiveConfigPropertyType::Struct: PropertyTypeStr = TEXT("Struct"); break;
		}

		FString TagsStr = FString::JoinBy(Def.Tags, TEXT(";"), [](const FName& Tag) { return Tag.ToString(); });

		CsvContent += FString::Printf(TEXT("%s,%s,%s,%s,%s\n"),
			*EscapeCSV(Def.PropertyName.ToString()),
			*EscapeCSV(Def.Value),
			*EscapeCSV(PropertyTypeStr),
			*EscapeCSV(Def.Description),
			*EscapeCSV(TagsStr)
		);
	}

	if (FFileHelper::SaveStringToFile(CsvContent, *ExportPath))
	{
		UE_LOG(LogLiveConfig, Log, TEXT("Successfully exported %d properties to: %s"), AllProperties.Num(), *ExportPath);
	}
	else
	{
		UE_LOG(LogLiveConfig, Error, TEXT("Failed to save exported CSV to: %s"), *ExportPath);
		return 1;
	}

	return 0;
}

FString ULiveConfigExportCommandlet::EscapeCSV(const FString& InString)
{
	if (InString.Contains(TEXT(",")) || InString.Contains(TEXT("\"")) || InString.Contains(TEXT("\n")))
	{
		FString Escaped = InString;
		Escaped.ReplaceInline(TEXT("\""), TEXT("\"\""));
		return FString::Printf(TEXT("\"%s\""), *Escaped);
	}
	return InString;
}
