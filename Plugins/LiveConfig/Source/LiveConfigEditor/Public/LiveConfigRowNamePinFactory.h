#pragma once

#include "CoreMinimal.h"
#include "EdGraphUtilities.h"

class FLiveConfigRowNamePinFactory : public FGraphPanelPinFactory
{
public:
    virtual TSharedPtr<class SGraphPin> CreatePin(class UEdGraphPin* Pin) const override;
};

