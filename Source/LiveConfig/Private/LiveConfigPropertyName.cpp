#include "LiveConfigPropertyName.h"
#include "LiveConfigTypes.h"
#include "LiveConfigSystem.h"

FLiveConfigProperty::FLiveConfigProperty(FName InPropertyName, bool bRedirect) : PropertyName(InPropertyName)
{ 
	if (bRedirect)
	{
		ULiveConfigSystem::Get().RedirectPropertyName(*this);
	}
}

FLiveConfigProperty::FLiveConfigProperty(const FString& InPropertyStr, bool bRedirect) : PropertyName(*InPropertyStr)
{
	if (bRedirect)
	{
		ULiveConfigSystem::Get().RedirectPropertyName(*this);
	}
}

FLiveConfigProperty FLiveConfigProperty::Request(FName InPropertyName)
{
	FLiveConfigProperty Property(InPropertyName);
	ULiveConfigSystem::Get().RedirectPropertyName(Property);
	return Property;
}

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

		ULiveConfigSystem::Get().RedirectPropertyName(*this);

		return true;
	}
	return false;
}

void FLiveConfigProperty::PostSerialize(const FArchive& Ar)
{
	if (Ar.IsLoading() && Ar.IsPersistent())
	{
		ULiveConfigSystem::Get().RedirectPropertyName(*this);
	}

	if (Ar.IsSaving())
	{
		if (IsValid())
		{
			// mark for searching in the asset registry
			Ar.MarkSearchableName(StaticStruct(), PropertyName);
		}
	}
}