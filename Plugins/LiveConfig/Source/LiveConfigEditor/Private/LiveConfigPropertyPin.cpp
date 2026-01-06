#include "LiveConfigPropertyPin.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "K2Node_LiveConfigLookup.h"
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

    TOptional<ELiveConfigPropertyType> FilterType;
    if (UEdGraphNode* Node = GraphPinObj->GetOwningNode())
    {
        if (UK2Node_CallFunction* CallFunctionNode = Cast<UK2Node_CallFunction>(Node))
        {
            if (UFunction* TargetFunction = CallFunctionNode->GetTargetFunction())
            {
                if (TargetFunction->GetOutermost()->GetName().Contains(TEXT("LiveConfig")))
                {
                    FName FunctionName = TargetFunction->GetFName();
                    if (FunctionName == TEXT("IsFeatureEnabled"))
                    {
                        FilterType = ELiveConfigPropertyType::Bool;
                    }
                    else if (FunctionName == TEXT("GetValue"))
                    {
                        FilterType = ELiveConfigPropertyType::Float;
                    }
                    else if (FunctionName == TEXT("GetIntValue"))
                    {
                        FilterType = ELiveConfigPropertyType::Int;
                    }
                    else if (FunctionName == TEXT("GetStringValue"))
                    {
                        FilterType = ELiveConfigPropertyType::String;
                    }
                }
            }
        }
        else if (UK2Node_LiveConfigLookup* LookupNode = Cast<UK2Node_LiveConfigLookup>(Node))
        {
            // No filter for the lookup node itself as it handles all types
        }
    }
    
    return SNew(SLiveConfigPropertyCombo)
        .Visibility(this, &SGraphPin::GetDefaultValueVisibility)
        .ReadOnly(false)
        .FilterType(FilterType)
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

