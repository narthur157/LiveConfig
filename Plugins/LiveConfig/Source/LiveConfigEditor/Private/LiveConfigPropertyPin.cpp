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

    if (GraphPinObj)
    {
        FLiveConfigProperty::StaticStruct()->ImportText(*GraphPinObj->DefaultValue, &CurrentProperty, nullptr, 0, nullptr, FLiveConfigProperty::StaticStruct()->GetName());
    }
}

TSharedRef<SWidget> SLiveConfigPropertyPin::GetDefaultValueWidget()
{
    if (!GraphPinObj)
    {
        return SNullWidget::NullWidget;
    }

    TOptional<ELiveConfigPropertyType> FilterType;
    UScriptStruct* StructFilter = nullptr;
    if (UEdGraphNode* Node = GraphPinObj->GetOwningNode())
    {
        if (UK2Node_LiveConfigLookup* LookupNode = Cast<UK2Node_LiveConfigLookup>(Node))
        {
            if (UEdGraphPin* ValuePin = LookupNode->GetValuePin())
            {
                if (ValuePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
                {
                    FilterType = ELiveConfigPropertyType::Bool;
                }
                else if (ValuePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int)
                {
                    FilterType = ELiveConfigPropertyType::Int;
                }
                else if (ValuePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Float || 
                    ValuePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Real || ValuePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Double)
                {
                    FilterType = ELiveConfigPropertyType::Float;
                }
                else if (ValuePin->PinType.PinCategory == UEdGraphSchema_K2::PC_String)
                {
                    FilterType = ELiveConfigPropertyType::String;
                }
                else if (ValuePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
                {
                    FilterType = ELiveConfigPropertyType::Struct;
                    StructFilter = Cast<UScriptStruct>(ValuePin->PinType.PinSubCategoryObject.Get());
                }
            }
        }
    }
    
    return SNew(SLiveConfigPropertyCombo)
        .Visibility(this, &SGraphPin::GetDefaultValueVisibility)
        .ReadOnly(false)
        .FilterType(FilterType)
        .StructFilter(StructFilter)
        .OnPropertyChanged(this, &SLiveConfigPropertyPin::OnPropertyChanged)
        .Property(this, &SLiveConfigPropertyPin::GetCurrentProperty);

}

void SLiveConfigPropertyPin::OnPropertyChanged(FLiveConfigProperty NewProperty)
{
    SetProperty(NewProperty);
}

FLiveConfigProperty SLiveConfigPropertyPin::GetCurrentProperty() const
{
    FLiveConfigProperty Property;
    if (GraphPinObj)
    {
        FLiveConfigProperty::StaticStruct()->ImportText(*GraphPinObj->DefaultValue, &Property, nullptr, 0, nullptr, FLiveConfigProperty::StaticStruct()->GetName());
    }
    return Property;
}

void SLiveConfigPropertyPin::SetProperty(FLiveConfigProperty NewProperty)
{
    if (!GraphPinObj)
    {
        return;
    }

    FLiveConfigProperty LiveConfigProperty(NewProperty);
    
    FString DefaultValue;
    FLiveConfigProperty::StaticStruct()->ExportText(DefaultValue, &LiveConfigProperty, &LiveConfigProperty, nullptr, 0, nullptr);

    if (GraphPinObj->DefaultValue == DefaultValue)
    {
        return;
    }

    // Undo/redo support
    const FScopedTransaction Transaction(LOCTEXT("ChangeDefaultValue", "Change Pin Default Value"));
    
    // Format the struct default value
    GraphPinObj->Modify();
    GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, DefaultValue);
    
    CurrentProperty = NewProperty;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE

