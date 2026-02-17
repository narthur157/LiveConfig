// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#include "LiveConfigPropertyTreeSubsystem.h"
#include "LiveConfigSystem.h"
#include "Engine/Engine.h"
#include "Misc/ComparisonUtility.h"

ULiveConfigPropertyTreeSubsystem& ULiveConfigPropertyTreeSubsystem::Get()
{
	check(GEditor);
	ULiveConfigPropertyTreeSubsystem* System = GEditor->GetEditorSubsystem<ULiveConfigPropertyTreeSubsystem>();
	check(System);
	
	return *System;
}

void ULiveConfigPropertyTreeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	ULiveConfigSystem& System = ULiveConfigSystem::Get();
	PropertiesUpdatedHandle = System.OnPropertiesUpdated.AddUObject(this, &ULiveConfigPropertyTreeSubsystem::RebuildFromSystem);
	
	RebuildFromSystem();
}

void ULiveConfigPropertyTreeSubsystem::Deinitialize()
{
	if (PropertiesUpdatedHandle.IsValid())
	{
		if (GEngine)
		{
			if (ULiveConfigSystem* System = GEngine->GetEngineSubsystem<ULiveConfigSystem>())
			{
				System->OnPropertiesUpdated.Remove(PropertiesUpdatedHandle);
			}
		}

		PropertiesUpdatedHandle.Reset();
	}

	Super::Deinitialize();
}

void ULiveConfigPropertyTreeSubsystem::RebuildFromSystem()
{
	if (!GEngine)
	{
		return;
	}

	ULiveConfigSystem* System = GEngine->GetEngineSubsystem<ULiveConfigSystem>();
	if (!System)
	{
		return;
	}

	Rebuild(System->GetAllProperties());
}

void ULiveConfigPropertyTreeSubsystem::Rebuild(const TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition>& InDefs)
{
	RootNodes.Reset();
	NodeByPath.Reset();
	Definitions.Reset();
	StructByName.Reset();

	Definitions.Reserve(InDefs.Num());
	for (const auto& Pair : InDefs)
	{
		Definitions.Add(MakeShared<FLiveConfigPropertyDefinition>(Pair.Value));
	}

	Definitions.Sort([](const TSharedPtr<FLiveConfigPropertyDefinition>& A, const TSharedPtr<FLiveConfigPropertyDefinition>& B)
	{
		return UE::ComparisonUtility::CompareNaturalOrder(A->PropertyName.ToString(), B->PropertyName.ToString()) < 0;
	});

	for (const auto& Def : Definitions)
	{
		if (Def->PropertyType == ELiveConfigPropertyType::Struct)
		{
			StructByName.Add(Def->PropertyName.ToString(), Def);
		}
	}

	TFunction<TSharedPtr<FLiveConfigPropertyModelNode>(const FString&)> GetOrCreateFolder;
	GetOrCreateFolder = [&](const FString& FolderPath) -> TSharedPtr<FLiveConfigPropertyModelNode>
	{
		if (TSharedPtr<FLiveConfigPropertyModelNode>* FoundFolder = NodeByPath.Find(FolderPath))
		{
			return *FoundFolder;
		}

		TSharedRef<FLiveConfigPropertyModelNode> NewNode = MakeShared<FLiveConfigPropertyModelNode>();
		NewNode->FullPath = FolderPath;

		if (TSharedPtr<FLiveConfigPropertyDefinition>* StructDef = StructByName.Find(FolderPath))
		{
			NewNode->PropertyDefinition = *StructDef;
		}

		int32 LastDot;
		if (FolderPath.FindLastChar('.', LastDot))
		{
			NewNode->DisplayName = FolderPath.RightChop(LastDot + 1);
			TSharedPtr<FLiveConfigPropertyModelNode> ParentFolder = GetOrCreateFolder(FolderPath.Left(LastDot));
			if (ParentFolder.IsValid())
			{
				ParentFolder->Children.Add(NewNode);
				NewNode->Parent = ParentFolder;
			}
		}
		else
		{
			NewNode->DisplayName = FolderPath;
			RootNodes.Add(NewNode);
		}

		TSharedPtr<FLiveConfigPropertyModelNode> NewNodePtr = NewNode;
		NodeByPath.Add(FolderPath, NewNodePtr);
		return NewNodePtr;
	};

	for (const auto& Def : Definitions)
	{
		const FString FullName = Def->PropertyName.ToString();

		if (Def->PropertyType == ELiveConfigPropertyType::Struct && NodeByPath.Contains(FullName))
		{
			continue;
		}

		int32 LastDot;
		if (FullName.FindLastChar('.', LastDot))
		{
			const FString FolderPath = FullName.Left(LastDot);
			const FString DisplayName = FullName.RightChop(LastDot + 1);
			TSharedPtr<FLiveConfigPropertyModelNode> ParentFolder = GetOrCreateFolder(FolderPath);

			TSharedRef<FLiveConfigPropertyModelNode> NewNode = MakeShared<FLiveConfigPropertyModelNode>();
			NewNode->DisplayName = DisplayName;
			NewNode->FullPath = FullName;
			NewNode->PropertyDefinition = Def;

			if (ParentFolder.IsValid())
			{
				NewNode->Parent = ParentFolder;
				ParentFolder->Children.Add(NewNode);
			}

			NodeByPath.Add(FullName, NewNode);
		}
		else
		{
			TSharedRef<FLiveConfigPropertyModelNode> NewNode = MakeShared<FLiveConfigPropertyModelNode>();
			NewNode->DisplayName = FullName;
			NewNode->FullPath = FullName;
			NewNode->PropertyDefinition = Def;

			RootNodes.Add(NewNode);
			NodeByPath.Add(FullName, NewNode);
		}
	}
}
