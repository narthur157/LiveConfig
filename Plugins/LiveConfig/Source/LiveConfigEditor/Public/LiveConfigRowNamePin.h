#pragma once

#include "CoreMinimal.h"
#include "KismetPins/SGraphPinStructInstance.h"
#include "LiveConfigSystem.h"

class SLiveConfigRowNamePin : public SGraphPinStructInstance
{
public:
    SLATE_BEGIN_ARGS(SLiveConfigRowNamePin) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);

protected:
    /** Returns the widget to display when the pin is not connected */
    virtual TSharedRef<SWidget> GetDefaultValueWidget() override;

private:
    /** Handle when the row name changes */
    void OnRowNameChanged(FLiveConfigRowName NewRowName);

    /** Get the current row name from the pin */
    FLiveConfigRowName GetCurrentRowName() const;

    /** Set the row name on the pin */
    void SetRowName(FLiveConfigRowName NewRowName);

    FLiveConfigRowName CurrentRowName;
};

