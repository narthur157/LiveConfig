#include "LiveConfigRowNamePin.h"
#include "LiveConfigRowPicker.h"
#include "EdGraphSchema_K2.h"
#include "LiveConfigPropertyName.h"
#include "LiveConfigRowCombo.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "LiveConfigSystem.h"
#include "LiveConfigRowPicker.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "LiveConfigRowNamePin"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SLiveConfigRowNamePin::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
    SGraphPinStructInstance::Construct(SGraphPinStructInstance::FArguments(), InGraphPinObj);
}

TSharedRef<SWidget> SLiveConfigRowNamePin::GetDefaultValueWidget()
{
    if (!GraphPinObj)
    {
        return SNullWidget::NullWidget;
    }
    
    return SNew(SLiveConfigRowCombo)
        .Visibility(this, &SGraphPin::GetDefaultValueVisibility)
        .ReadOnly(false)
        .OnRowNameChanged(this, &SLiveConfigRowNamePin::OnRowNameChanged)
        .RowName(this , &SLiveConfigRowNamePin::GetCurrentRowName);

}

void SLiveConfigRowNamePin::OnRowNameChanged(FLiveConfigRowName NewRowName)
{
    SetRowName(NewRowName);
}

FLiveConfigRowName SLiveConfigRowNamePin::GetCurrentRowName() const
{
    return CurrentRowName;

}

void SLiveConfigRowNamePin::SetRowName(FLiveConfigRowName NewRowName)
{
    if (NewRowName == CurrentRowName)
    {
        return;
    }

    CurrentRowName = NewRowName;

    FLiveConfigRowName LiveConfigProperty(NewRowName);
    
    FString DefaultValue;
    FLiveConfigRowName::StaticStruct()->ExportText(DefaultValue, &LiveConfigProperty, &LiveConfigProperty, nullptr, 0, nullptr);

    // Undo/redo support
    const FScopedTransaction Transaction(LOCTEXT("ChangeDefaultValue", "Change Pin Default Value"));
    
    // Format the struct default value
    GraphPinObj->Modify();
    GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, DefaultValue);
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
