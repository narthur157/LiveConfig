#include "LiveConfigTypes.h"

DEFINE_LOG_CATEGORY(LogLiveConfig);

bool FLiveConfigPropertyDefinition::IsValid() const
{
	return PropertyName.IsValid();
}

void FLiveConfigCache::Reset()
{
	FloatValues.Reset();
	IntValues.Reset();
	StringValues.Reset();
	BoolValues.Reset();
}

void FLiveConfigCache::SetValue(const FLiveConfigPropertyDefinition& InValue)
{
	switch (InValue.PropertyType) {
	case ELiveConfigPropertyType::String:
		StringValues.Add(InValue.PropertyName, InValue.Value);
		break;
	case ELiveConfigPropertyType::Int:
		IntValues.Add(InValue.PropertyName, FCString::Atoi(*InValue.Value));
		break;
	case ELiveConfigPropertyType::Float:
		FloatValues.Add(InValue.PropertyName, FCString::Atof(*InValue.Value));
		break;
	case ELiveConfigPropertyType::Bool:
		BoolValues.Add(InValue.PropertyName, InValue.Value.Contains("true"));
		break;
	}
}

void FLiveConfigCache::BuildConfig(const TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition>& PropertyDefinitions,
	const FLiveConfigProfile& EnvironmentProfile, const FLiveConfigProfile& ActiveProfile, FLiveConfigCache& OutCache)
{
	OutCache.Reset();
	
	for (const TPair<FLiveConfigProperty, FLiveConfigPropertyDefinition>& Pair : PropertyDefinitions)
	{
		OutCache.SetValue(Pair.Value);	
	}
	
	auto AddProfileLayer = [&](const FLiveConfigProfile& Profile)
	{
		for (const TPair<FLiveConfigProperty, FString>& Pair : Profile.Overrides)
		{
			FLiveConfigPropertyDefinition TempDef;
			TempDef.PropertyName = Pair.Key;
			TempDef.Value = Pair.Value;
			OutCache.SetValue(TempDef);
		}
	};
	
	// We could add more layers here/support custom layering, but complex layering creates UX issues
	AddProfileLayer(EnvironmentProfile);
	AddProfileLayer(ActiveProfile);
}
