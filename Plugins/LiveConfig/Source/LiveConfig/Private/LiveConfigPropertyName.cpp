#include "LiveConfigPropertyName.h"

bool FLiveConfigProperty::ExportTextItem(FString& ValueStr, FLiveConfigProperty const& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const
{
	ValueStr += PropertyName.ToString();
	return true;
}

bool FLiveConfigProperty::ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText)
{
	FString ImportedString;
	const TCHAR* NewBuffer = FPropertyHelpers::ReadToken(Buffer, ImportedString, true);
	if (NewBuffer)
	{
		PropertyName = FName(*ImportedString);
		Buffer = NewBuffer;
		return true;
	}
	return false;
}