// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#include "Blueprint/LiveConfigPropertyPinFactory.h"

#include "EdGraphSchema_K2.h"
#include "LiveConfigPropertyName.h"
#include "Blueprint/SLiveConfigPropertyPin.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

TSharedPtr<class SGraphPin> FLiveConfigPropertyPinFactory::CreatePin(class UEdGraphPin* Pin) const
{
    if (Pin == nullptr)
    {
        return nullptr;
    }

    // Check if the pin is the FLiveConfigRowName struct type
    if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct &&
        Pin->PinType.PinSubCategoryObject == FLiveConfigProperty::StaticStruct())
    {
        return SNew(SLiveConfigPropertyPin, Pin);
    }

    return nullptr;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
