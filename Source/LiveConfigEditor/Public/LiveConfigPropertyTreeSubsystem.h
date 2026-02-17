// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#pragma once

#include "CoreMinimal.h"
#include "LiveConfigTypes.h"
#include "EditorSubsystem.h"
#include "LiveConfigPropertyTreeSubsystem.generated.h"

// Canonical, shared tree node built from live config property definitions.
// This model is owned by the subsystem and should be treated as read-only by views.
struct FLiveConfigPropertyModelNode : public TSharedFromThis<FLiveConfigPropertyModelNode>
{
	FString DisplayName;
	FString FullPath;
	TSharedPtr<FLiveConfigPropertyDefinition> PropertyDefinition;
	TArray<TSharedRef<FLiveConfigPropertyModelNode>> Children;
	TWeakPtr<FLiveConfigPropertyModelNode> Parent;

	bool IsProperty() const { return PropertyDefinition.IsValid() && PropertyDefinition->PropertyType != ELiveConfigPropertyType::Struct; }
	bool IsStruct() const { return PropertyDefinition.IsValid() && PropertyDefinition->PropertyType == ELiveConfigPropertyType::Struct; }
};

// Editor-only subsystem that owns the property tree and shared definition list,
// rebuilt on LiveConfigSystem updates for all manager views to consume.
UCLASS()
class LIVECONFIGEDITOR_API ULiveConfigPropertyTreeSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	static ULiveConfigPropertyTreeSubsystem& Get();
	
	// USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// ~USubsystem

	// Root nodes for the tree (folders/structs/properties).
	const TArray<TSharedRef<FLiveConfigPropertyModelNode>>& GetRootNodes() const { return RootNodes; }
	
	// Fast lookup of nodes by full property path.
	const TMap<FString, TSharedPtr<FLiveConfigPropertyModelNode>>& GetNodeByPath() const { return NodeByPath; }
	
	// Flat, alphabetically (natural) sorted list of shared property definitions backing the tree.
	const TArray<TSharedPtr<FLiveConfigPropertyDefinition>>& GetDefinitions() const { return Definitions; }

	void RebuildFromSystem();

	// Shared definitions backing the canonical tree.
	TArray<TSharedPtr<FLiveConfigPropertyDefinition>> Definitions;

private:
	void Rebuild(const TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition>& InDefs);

	TArray<TSharedRef<FLiveConfigPropertyModelNode>> RootNodes;
	TMap<FString, TSharedPtr<FLiveConfigPropertyModelNode>> NodeByPath;
	TMap<FString, TSharedPtr<FLiveConfigPropertyDefinition>> StructByName;
	FDelegateHandle PropertiesUpdatedHandle;
};
