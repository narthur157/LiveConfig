// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#pragma once

#include "CoreMinimal.h"
#include "KismetPins/SGraphPinStructInstance.h"
#include "LiveConfigSystem.h"

class SLiveConfigPropertyPin : public SGraphPinStructInstance
{
public:
    SLATE_BEGIN_ARGS(SLiveConfigPropertyPin) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);

protected:
    /** Returns the widget to display when the pin is not connected */
    virtual TSharedRef<SWidget> GetDefaultValueWidget() override;

private:
    /** Handle when the property changes */
    void OnPropertyChanged(FLiveConfigProperty NewProperty);

    /** Get the current property from the pin */
    FLiveConfigProperty GetCurrentProperty() const;

    /** Set the property on the pin */
    void SetProperty(FLiveConfigProperty NewProperty);

    FLiveConfigProperty CurrentProperty;
};


