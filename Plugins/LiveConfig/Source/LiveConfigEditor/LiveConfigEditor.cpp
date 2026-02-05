#include "LiveConfigEditor.h"

#include "EdGraphUtilities.h"
#include "LiveConfigPropertyName.h"
#include "LiveConfigPropertyStyle.h"
#include "LiveConfigPropertyCustomization.h"
#include "LiveConfigPropertyPinFactory.h"
#include "LiveConfigTypes.h"
#include "LiveConfigBlueprintExtensions.h"
#include "GraphEditorModule.h"
#include "PropertyManager/SLiveConfigPropertyManager.h"
#include "LiveConfigGameSettings.h"
#include "Widgets/Docking/SDockTab.h"
#include "Logging/MessageLog.h"
#include "Logging/TokenizedMessage.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/FileHelper.h"

#define LOCTEXT_NAMESPACE "FLiveConfigEditorModule"

static const FName LiveConfigPropertyManagerTabId("LiveConfigPropertyManager");

void FLiveConfigEditorModule::StartupModule()
{
	UE_LOG(LogLiveConfig, Log, TEXT("LiveConfigEditorModule::StartupModule()"));

	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// Register customization for FLiveConfigProperty
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FLiveConfigProperty::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FLiveConfigPropertyCustomization::MakeInstance)
	);

	auto PropertyPinFactory = MakeShareable(new FLiveConfigPropertyPinFactory());
	FEdGraphUtilities::RegisterVisualPinFactory(PropertyPinFactory);
	FLiveConfigPropertyStyle::Initialize();

	// Initialize blueprint extensions for "Promote to Live Config"
	FLiveConfigBlueprintExtensions::Initialize();

	// Register FExtender fallback
	FGraphEditorModule& GraphEditorModule = FModuleManager::LoadModuleChecked<FGraphEditorModule>("GraphEditor");
	TArray<FGraphEditorModule::FGraphEditorMenuExtender_SelectedNode>& MenuExtenderList = GraphEditorModule.GetAllGraphEditorContextMenuExtender();
	MenuExtenderList.Add(FGraphEditorModule::FGraphEditorMenuExtender_SelectedNode::CreateStatic(&FLiveConfigBlueprintExtensions::OnExtendPinMenu));

	RegisterTabSpawners();

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FLiveConfigEditorModule::RegisterMenus));

	CheckForMissingTags();
}

void FLiveConfigEditorModule::CheckForMissingTags()
{
	TArray<FName> MissingTags;
	SLiveConfigPropertyManager::GetMissingTags(MissingTags);

	if (MissingTags.Num() > 0)
	{
		FMessageLog LiveConfigLog("LiveConfig");
		
		TSharedRef<FTokenizedMessage> Message = LiveConfigLog.Warning();
		Message->AddToken(FTextToken::Create(FText::Format(
			LOCTEXT("MissingTagsWarning", "Found {0} tags used in properties that are not in the Known Tags list."),
			FText::AsNumber(MissingTags.Num())
		)));
		
		Message->AddToken(FActionToken::Create(
			LOCTEXT("FixMissingTags", "Fix"),
			LOCTEXT("FixMissingTagsToolTip", "Import missing tags into the Known Tags list"),
			FOnActionTokenExecuted::CreateRaw(this, &FLiveConfigEditorModule::FixMissingTags, MissingTags),
			true
		));

		LiveConfigLog.Open(EMessageSeverity::Warning, true);
	}
}

void FLiveConfigEditorModule::FixMissingTags(TArray<FName> MissingTags)
{
	ULiveConfigGameSettings* Settings = GetMutableDefault<ULiveConfigGameSettings>();
	if (Settings)
	{
		TArray<FName> NewKnownTags = Settings->KnownTags;
		for (const FName& Tag : MissingTags)
		{
			NewKnownTags.AddUnique(Tag);
		}
		SLiveConfigPropertyManager::SaveKnownTags(NewKnownTags);
		
		FMessageLog("LiveConfig").Info(LOCTEXT("MissingTagsFixed", "Successfully imported missing tags."));
	}
}

void FLiveConfigEditorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UnregisterTabSpawners();

	// Shutdown blueprint extensions
	FLiveConfigBlueprintExtensions::Shutdown();

	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.UnregisterCustomPropertyTypeLayout(FLiveConfigPropertyDefinition::StaticStruct()->GetFName());
	PropertyModule.UnregisterCustomPropertyTypeLayout(FLiveConfigProperty::StaticStruct()->GetFName());
}

void FLiveConfigEditorModule::OpenPropertyManager(FLiveConfigProperty FocusProperty)
{
	PropertyToFocus = FocusProperty;
	TSharedPtr<SDockTab> Tab = FGlobalTabmanager::Get()->TryInvokeTab(LiveConfigPropertyManagerTabId);
	if (Tab.IsValid())
	{
		TSharedRef<SLiveConfigPropertyManager> Manager = StaticCastSharedRef<SLiveConfigPropertyManager>(Tab->GetContent());
		if (PropertyToFocus.IsValid())
		{
			Manager->ScrollToProperty(PropertyToFocus);
		}
		PropertyToFocus = FLiveConfigProperty();
	}
}

void FLiveConfigEditorModule::RegisterTabSpawners()
{
	FTabSpawnerEntry& Spawner = FGlobalTabmanager::Get()->RegisterTabSpawner(LiveConfigPropertyManagerTabId, FOnSpawnTab::CreateRaw(this, &FLiveConfigEditorModule::SpawnPropertyManagerTab))
		.SetDisplayName(LOCTEXT("PropertyManagerTabTitle", "Live Config Property Manager"))
		.SetMenuType(ETabSpawnerMenuType::Enabled);
}

void FLiveConfigEditorModule::UnregisterTabSpawners()
{
	FGlobalTabmanager::Get()->UnregisterTabSpawner(LiveConfigPropertyManagerTabId);
}

TSharedRef<SDockTab> FLiveConfigEditorModule::SpawnPropertyManagerTab(const FSpawnTabArgs& Args)
{
	TSharedRef<SLiveConfigPropertyManager> Manager = SNew(SLiveConfigPropertyManager);
	if (PropertyToFocus.IsValid())
	{
		Manager->ScrollToProperty(PropertyToFocus);
		PropertyToFocus = FLiveConfigProperty();
	}

	return SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		[
			Manager
		];
}

void FLiveConfigEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* PlayMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
	if (PlayMenu)
	{
		
		FToolMenuSection& Section = PlayMenu->AddSection("LiveConfig", LOCTEXT("LiveConfigSection", "Live Config"));
		FToolMenuEntry Entry = FToolMenuEntry::InitToolBarButton(
			"OpenLiveConfigManager",
			FUIAction(FExecuteAction::CreateRaw(this, &FLiveConfigEditorModule::OpenPropertyManager, FLiveConfigProperty())),
			LOCTEXT("OpenLiveConfigManager", "Live Config"),
			LOCTEXT("OpenLiveConfigManagerTooltip", "Open the Live Config Property Manager"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Settings")
		);
		Entry.SetCommandList(nullptr);
		Entry.StyleNameOverride = "CalloutToolbar";
		Section.AddEntry(Entry);

		FToolMenuEntry ExportEntry = FToolMenuEntry::InitToolBarButton(
			"ExportLiveConfigCsv",
			FUIAction(FExecuteAction::CreateRaw(this, &FLiveConfigEditorModule::OnExportCsv)),
			LOCTEXT("ExportLiveConfigCsv", "Export CSV"),
			LOCTEXT("ExportLiveConfigCsvTooltip", "Export Live Config data to a Google Sheets-friendly CSV file"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Save")
		);
		Section.AddEntry(ExportEntry);
	}
}

void FLiveConfigEditorModule::OnExportCsv()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return;
	}

	TArray<FString> SaveFilenames;
	const bool bOpened = DesktopPlatform->SaveFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		LOCTEXT("ExportCsvDialogTitle", "Export Live Config to CSV").ToString(),
		FPaths::ProjectSavedDir(),
		TEXT("LiveConfig.csv"),
		TEXT("CSV files (*.csv)|*.csv"),
		EFileDialogFlags::None,
		SaveFilenames
	);

	if (bOpened && SaveFilenames.Num() > 0)
	{
		FString CsvContent = TEXT("Name,Value,Type,Description,Tags\n");
		
		const TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition>& AllProperties = ULiveConfigSystem::Get().GetAllProperties();
		
		for (auto& Pair : AllProperties)
		{
			const FLiveConfigPropertyDefinition& Def = Pair.Value;
			
			// Escape function to make it Google Sheets friendly
			auto EscapeCSV = [](const FString& InString) -> FString
			{
				if (InString.Contains(TEXT(",")) || InString.Contains(TEXT("\"")) || InString.Contains(TEXT("\n")))
				{
					FString Escaped = InString;
					Escaped.ReplaceInline(TEXT("\""), TEXT("\"\""));
					return FString::Printf(TEXT("\"%s\""), *Escaped);
				}
				return InString;
			};

			FString PropertyTypeStr;
			switch(Def.PropertyType)
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

		if (FFileHelper::SaveStringToFile(CsvContent, *SaveFilenames[0]))
		{
			FMessageLog("LiveConfig").Info(FText::Format(LOCTEXT("ExportSuccess", "Successfully exported Live Config to {0}"), FText::FromString(SaveFilenames[0])));
		}
		else
		{
			FMessageLog("LiveConfig").Error(FText::Format(LOCTEXT("ExportFailed", "Failed to save CSV to {0}"), FText::FromString(SaveFilenames[0])));
		}
	}
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FLiveConfigEditorModule, LiveConfigEditor);