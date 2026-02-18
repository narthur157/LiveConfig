// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#pragma once

#include "CoreMinimal.h"
#include "EdGraphUtilities.h"

class FLiveConfigPropertyPinFactory : public FGraphPanelPinFactory
{
public:
    virtual TSharedPtr<class SGraphPin> CreatePin(class UEdGraphPin* Pin) const override;
};


