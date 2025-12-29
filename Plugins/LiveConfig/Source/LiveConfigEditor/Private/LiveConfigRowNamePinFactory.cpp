#include "LiveConfigRowNamePinFactory.h"

#include "EdGraphSchema_K2.h"
#include "LiveConfigPropertyName.h"
#include "LiveConfigRowNamePin.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

TSharedPtr<class SGraphPin> FLiveConfigRowNamePinFactory::CreatePin(class UEdGraphPin* Pin) const
{
    if (Pin == nullptr)
    {
        return nullptr;
    }

    // Check if the pin is the FLiveConfigRowName struct type
    if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct &&
        Pin->PinType.PinSubCategoryObject == FLiveConfigRowName::StaticStruct())
    {
        return SNew(SLiveConfigRowNamePin, Pin);
    }

    return nullptr;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
