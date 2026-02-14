#pragma once

#include "CoreMinimal.h"
#include "EdGraphUtilities.h"

class FLiveConfigPropertyPinFactory : public FGraphPanelPinFactory
{
public:
    virtual TSharedPtr<class SGraphPin> CreatePin(class UEdGraphPin* Pin) const override;
};


