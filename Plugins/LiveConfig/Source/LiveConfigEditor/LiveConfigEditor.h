#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "LiveConfigPropertyName.h"

class FLiveConfigEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

	/** Opens the property manager window */
	void OpenPropertyManager(FLiveConfigProperty FocusProperty = FLiveConfigProperty());

private:
	void CheckForMissingTags();
	void FixMissingTags(TArray<FName> MissingTags);

	FLiveConfigProperty PropertyToFocus;
	void RegisterTabSpawners();
	void UnregisterTabSpawners();

	TSharedRef<SDockTab> SpawnPropertyManagerTab(const FSpawnTabArgs& Args);

	void RegisterMenus();
};
