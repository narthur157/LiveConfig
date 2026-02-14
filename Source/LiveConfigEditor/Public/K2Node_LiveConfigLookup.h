#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "LiveConfigPropertyName.h"
#include "K2Node_LiveConfigLookup.generated.h"

UCLASS()
class LIVECONFIGEDITOR_API UK2Node_LiveConfigLookup : public UK2Node
{
	GENERATED_BODY()

public:
	UK2Node_LiveConfigLookup();

	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetMenuCategory() const override;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual bool IsNodePure() const override { return true; }
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual bool IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutConnectionMessage) const override;
	virtual void PostReconstructNode() override;
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const override;
	// End UK2Node interface

	/** Get the property pin */
	UEdGraphPin* GetPropertyPin() const;

	/** Get the output value pin */
	UEdGraphPin* GetValuePin() const;

	/** Update the output pin type based on the selected property */
	void UpdateOutputPinType();

	/** Default pin type to use when no property is selected */
	UPROPERTY()
	FEdGraphPinType DefaultPinType;

private:
	UFUNCTION(BlueprintPure, Category = "Live Config", BlueprintInternalUseOnly)
	static bool GetBoolValue(FLiveConfigProperty Property);

	UFUNCTION(BlueprintPure, Category = "Live Config", BlueprintInternalUseOnly)
	static float GetFloatValue(FLiveConfigProperty Property);

	UFUNCTION(BlueprintPure, Category = "Live Config", BlueprintInternalUseOnly)
	static int32 GetIntValue(FLiveConfigProperty Property);

	UFUNCTION(BlueprintPure, Category = "Live Config", BlueprintInternalUseOnly)
	static FString GetStringValue(FLiveConfigProperty Property);
	
	/**
	 * Gets a struct value from live config.
	 * Each property in the struct will be looked up as Property.PropertyName.
	 */
	UFUNCTION(BlueprintPure, CustomThunk, Category = "Live Config", meta = (CustomStructureParam = "OutStruct"), BlueprintInternalUseOnly)
	static void GetStructValue(FLiveConfigProperty Property, int32& OutStruct);

	static void Generic_GetStructValue(FLiveConfigProperty Property, UScriptStruct* Struct, void* OutStructPtr);

	DECLARE_FUNCTION(execGetStructValue);
	
	/** Name of the property pin */
	static FName PropertyPinName;

	/** Name of the value pin */
	static FName ValuePinName;
};
