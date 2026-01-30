#include "PropertyManager/SLiveConfigPropertyValueWidget.h"
#include "LiveConfigJson.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"
#include "StructViewerModule.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "LiveConfigEditor"

FLiveConfigStructFilter::FLiveConfigStructFilter()
{
	FModuleManager& ModuleManager = FModuleManager::Get();

	TArray<FModuleDiskInfo> AllModules;
	ModuleManager.FindModules(TEXT("*"), AllModules);

	FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	
	AllowedScripts.Add("/Script/CoreUObject.Vector");
	AllowedScripts.Add("/Script/CoreUObject.Vector2D");
	AllowedScripts.Add("/Script/CoreUObject.Transform");
	AllowedScripts.Add("/Script/CoreUObject.Color");

	for (const auto& ModuleInfo : AllModules)
	{
		FName ModuleName = ModuleInfo.Name;
		if (ModuleInfo.FilePath.IsEmpty())
		{
			continue;
		}
		if (!ModuleManager.ModuleExists(*ModuleName.ToString()))
		{
			continue;
		}
		
		if (ModuleName.ToString() == "LiveConfig")
		{
			continue;
		}
		
		const FString& ModuleFilenameRelative = ModuleInfo.FilePath;
		FString ModuleFilename = FPaths::ConvertRelativePathToFull(ModuleFilenameRelative);

		if (ModuleFilename.Contains(ProjectDir))
		{
			AllowedScripts.Add(TEXT("/Script/") + ModuleName.ToString());
		}
	}
}

bool FLiveConfigStructFilter::IsStructAllowed(const FStructViewerInitializationOptions& InInitOptions, const UScriptStruct* InStruct, TSharedRef<class FStructViewerFilterFuncs> InNode)
{
	if (!InStruct)
	{
		return false;
	}
	
	static const FName NAME_HiddenMetaTag = "Hidden";
	if (InStruct->HasMetaData(NAME_HiddenMetaTag))
	{
		return false;
	}

	FString PackageName = InStruct->GetStructPathName().ToString();
	return IsPackageAllowed(PackageName);
}

bool FLiveConfigStructFilter::IsUnloadedStructAllowed(const FStructViewerInitializationOptions& InInitOptions, const FSoftObjectPath& InStructPath, TSharedRef<class FStructViewerFilterFuncs> InNode)
{
	FString PackageName = InStructPath.GetLongPackageName();
	return IsPackageAllowed(PackageName);
}

bool FLiveConfigStructFilter::IsPackageAllowed(const FString& PackageName) const
{
	if (PackageName.StartsWith(TEXT("/Game/")))
	{
		return true;
	}

	for (const FString& AllowedScript : AllowedScripts)
	{
		if (PackageName.StartsWith(AllowedScript))
		{
			return true;
		}
	}

	return false;
}

void SLiveConfigPropertyValueWidget::PrivateRegisterAttributes(FSlateAttributeInitializer& AttributeInitializer)
{
}

SLATE_IMPLEMENT_WIDGET(SLiveConfigPropertyValueWidget);

void SLiveConfigPropertyValueWidget::Construct(const FArguments& InArgs)
{
	ValueAttribute = InArgs._Value;
	PropertyTypeAttribute = InArgs._PropertyType;
	bReadOnlyAttribute = InArgs._bReadOnly;
	OnValueChanged = InArgs._OnValueChanged;

	ChildSlot
	[
		SNew(SWidgetSwitcher)
		.WidgetIndex_Lambda([this]()
		{
			switch (PropertyTypeAttribute.Get())
			{
				case ELiveConfigPropertyType::Bool: return 1;
				case ELiveConfigPropertyType::Struct: return 2;
				default: return 0;
			}
		})
		+ SWidgetSwitcher::Slot()
		[
			SAssignNew(ValueTextBox, SEditableTextBox)
			.Text_Lambda([this]() { return FText::FromString(ValueAttribute.Get()); })
			.OnVerifyTextChanged(this, &SLiveConfigPropertyValueWidget::VerifyValueText)
			.OnTextCommitted(this, &SLiveConfigPropertyValueWidget::ValueTextCommitted)
			.IsReadOnly_Lambda([this]() { return bReadOnlyAttribute.Get(); })
		]
		+ SWidgetSwitcher::Slot()
		[
			SAssignNew(ValueCheckBox, SCheckBox)
			.IsChecked_Lambda([this]()
			{
				return ValueAttribute.Get().ToBool() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
			.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
			{
				FString NewVal = NewState == ECheckBoxState::Checked ? TEXT("true") : TEXT("false");
				OnValueChanged.ExecuteIfBound(NewVal);
			})
			.IsEnabled_Lambda([this]() { return !bReadOnlyAttribute.Get(); })
		]
		+ SWidgetSwitcher::Slot()
		[
			SAssignNew(StructComboButton, SComboButton)
			.IsEnabled_Lambda([this]() { return !bReadOnlyAttribute.Get(); })
			.OnGetMenuContent(this, &SLiveConfigPropertyValueWidget::OnGetStructPickerMenu)
			.ContentPadding(FMargin(4.0f, 2.0f))
			.ButtonContent()
			[
				SNew(STextBlock)
				.Text_Lambda([this]()
				{
					FString Val = ValueAttribute.Get();
					if (Val.IsEmpty())
					{
						return LOCTEXT("None", "None");
					}
					return FText::FromString(Val);
				})
			]
		]
	];
}

void SLiveConfigPropertyValueWidget::RequestFocus()
{
	ELiveConfigPropertyType PropertyType = PropertyTypeAttribute.Get();
	if (PropertyType == ELiveConfigPropertyType::Bool)
	{
		if (ValueCheckBox.IsValid())
		{
			FSlateApplication::Get().SetKeyboardFocus(ValueCheckBox.ToSharedRef(), EFocusCause::SetDirectly);
		}
	}
	else if (PropertyType == ELiveConfigPropertyType::Struct)
	{
		if (StructComboButton.IsValid())
		{
			FSlateApplication::Get().SetKeyboardFocus(StructComboButton.ToSharedRef(), EFocusCause::SetDirectly);
		}
	}
	else
	{
		if (ValueTextBox.IsValid())
		{
			FSlateApplication::Get().SetKeyboardFocus(ValueTextBox.ToSharedRef(), EFocusCause::SetDirectly);
		}
	}
}

TSharedRef<SWidget> SLiveConfigPropertyValueWidget::OnGetStructPickerMenu()
{
	FStructViewerModule& StructViewerModule = FModuleManager::LoadModuleChecked<FStructViewerModule>("StructViewer");

	FStructViewerInitializationOptions Options;
	Options.Mode = EStructViewerMode::StructPicker;
	Options.StructFilter = MakeShared<FLiveConfigStructFilter>();
	
	return SNew(SBox)
		.WidthOverride(300.0f)
		.HeightOverride(400.0f)
		[
			StructViewerModule.CreateStructViewer(Options, FOnStructPicked::CreateSP(this, &SLiveConfigPropertyValueWidget::OnStructPicked))
		];
}

void SLiveConfigPropertyValueWidget::OnStructPicked(const UScriptStruct* ChosenStruct)
{
	FSlateApplication::Get().DismissAllMenus();
	FString NewVal = ChosenStruct ? ChosenStruct->GetName() : "";
	OnValueChanged.ExecuteIfBound(NewVal);
}

bool SLiveConfigPropertyValueWidget::VerifyValueText(const FText& NewText, FText& OutError)
{
	FString NewVal = NewText.ToString();
	ELiveConfigPropertyType PropertyType = PropertyTypeAttribute.Get();
	
	if (PropertyType == ELiveConfigPropertyType::Int)
	{
		if (NewVal.IsEmpty() || NewVal == TEXT("-")) return true;
		if (!NewVal.IsNumeric() || NewVal.Contains(TEXT(".")))
		{
			OutError = LOCTEXT("ValueIntError", "Value must be a valid integer.");
			return false;
		}
	}
	else if (PropertyType == ELiveConfigPropertyType::Float)
	{
		if (NewVal.IsEmpty() || NewVal == TEXT("-") || NewVal == TEXT(".") || NewVal == TEXT("-.")) return true;
		if (!NewVal.IsNumeric())
		{
			OutError = LOCTEXT("ValueFloatError", "Value must be a valid number.");
			return false;
		}
	}
	return true;
}

void SLiveConfigPropertyValueWidget::ValueTextCommitted(const FText& NewText, ETextCommit::Type CommitType)
{
	FString NewVal = NewText.ToString();
	ELiveConfigPropertyType PropertyType = PropertyTypeAttribute.Get();

	if (PropertyType == ELiveConfigPropertyType::Int)
	{
		if (NewVal.IsEmpty() || NewVal == TEXT("-")) { NewVal = "0"; }
		else if (NewVal.IsNumeric() && !NewVal.Contains(TEXT("."))) { /* ok */ }
		else { return; }
	}
	else if (PropertyType == ELiveConfigPropertyType::Float)
	{
		if (NewVal.IsEmpty() || NewVal == TEXT("-") || NewVal == TEXT(".") || NewVal == TEXT("-.")) { NewVal = "0"; }
		else if (NewVal.IsNumeric()) { /* ok */ }
		else { return; }
	}
	
	OnValueChanged.ExecuteIfBound(NewVal);
}

#undef LOCTEXT_NAMESPACE
