#include "LiveConfigPropertyPin.h"
#include "EdGraphSchema_K2.h"
#include "LiveConfigPropertyName.h"
#include "LiveConfigPropertyCombo.h"

#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "LiveConfigPropertyPin"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SLiveConfigPropertyPin::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
    SGraphPinStructInstance::Construct(SGraphPinStructInstance::FArguments(), InGraphPinObj);
}

TSharedRef<SWidget> SLiveConfigPropertyPin::GetDefaultValueWidget()
{
    if (!GraphPinObj)
    {
        return SNullWidget::NullWidget;
    }
    
    return SNew(SLiveConfigPropertyCombo)
        .Visibility(this, &SGraphPin::GetDefaultValueVisibility)
        .ReadOnly(false)
        .OnPropertyChanged(this, &SLiveConfigPropertyPin::OnPropertyChanged)
        .Property(this , &SLiveConfigPropertyPin::GetCurrentProperty);

}

void SLiveConfigPropertyPin::OnPropertyChanged(FLiveConfigProperty NewProperty)
{
    SetProperty(NewProperty);
}

FLiveConfigProperty SLiveConfigPropertyPin::GetCurrentProperty() const
{
    return CurrentProperty;

}

void SLiveConfigPropertyPin::SetProperty(FLiveConfigProperty NewProperty)
{
    if (NewProperty == CurrentProperty)
    {
        return;
    }

    CurrentProperty = NewProperty;

    FLiveConfigProperty LiveConfigProperty(NewProperty);
    
    FString DefaultValue;
    FLiveConfigProperty::StaticStruct()->ExportText(DefaultValue, &LiveConfigProperty, &LiveConfigProperty, nullptr, 0, nullptr);

    // Undo/redo support
    const FScopedTransaction Transaction(LOCTEXT("ChangeDefaultValue", "Change Pin Default Value"));
    
    // Format the struct default value
    GraphPinObj->Modify();
    GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, DefaultValue);
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

