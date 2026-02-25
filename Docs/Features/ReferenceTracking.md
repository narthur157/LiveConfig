
# Reference Tracking and Renaming

LiveConfig uses Unreal's **Searchable Names** in the Asset Registry. When an `FLiveConfigProperty` is saved in any asset (Blueprint, Level, Data Asset), its name is registered as a searchable dependency.
- Use **find usages** on a property name in the LiveConfig editor to see all assets that reference it.

#### Renaming and Redirectors
If a property needs to be renamed, LiveConfig supports **Property Redirectors** via configuration.
1. Add a redirect entry in `DefaultGame.ini` (or through the LiveConfig settings):
   ```ini
   [/Script/LiveConfig.LiveConfigSystem]
   +PropertyRedirects=(OldName="Old.Property.Name", NewName="New.Property.Name")
   ```
2. When an asset containing the old property name is loaded, it will automatically be updated to the new name.
3. This system mirrors `GameplayTagRedirects`, ensuring that renames don't break existing references.

When using the `RenameProperty` function in C++, you can optionally pass `true` for the `bCreateRedirector` parameter to automatically add a redirect entry to the `PropertyRedirects` map. For structs, this will also recursively create redirectors for all member properties.
