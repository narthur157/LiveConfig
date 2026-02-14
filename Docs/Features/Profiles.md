# Config Profiles

Profiles are sets of property values that apply to a single world / session.
Profiles are replicated to clients.

## Uses
- Adjust property values to test the impact from changing a value
- Apply a profile specific to a particular map or game mode to adjust rules
- Create a tuning pack for speculative tuning changes that are not ready to be committed yet. 

## Creating profiles

Profiles are created implicitly and edited via the `LiveConfig.SetOverride <PropertyName> <Value>` command

Profiles are only saved when `LiveConfig.SaveProfile <ProfileName>` is called

A profile can be loaded via `LiveConfig.LoadProfile <ProfileName>`

## Limitations
Only 1 profile can be applied at a time. However, support could be added for a layering system.