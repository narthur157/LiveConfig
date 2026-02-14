## LiveConfig Data Structure 
### Directory Structure and Property Naming

The LiveConfig data is stored in the `Config/LiveConfig/` directory. The plugin uses the filesystem structure (folders and filenames) to define the hierarchy and naming of configuration properties.

- **Root Directory**: `Config/LiveConfig/`
- **Property Name Formation**: The property name is derived from the relative path of the JSON file within the `Config/LiveConfig/` directory. Folder separators are converted into dots (`.`), and the `.json` extension is removed.
- **Examples**:
    - `Config/LiveConfig/SomeProperty.json` -> `SomeProperty`
    - `Config/LiveConfig/Weapon/SMG/Damage.json` -> `Weapon.SMG.Damage`

### JSON File Schema

Each property is defined in its own JSON file with the following fields:

| Field | Type | Description |
| :--- | :--- | :--- |
| `propertyName` | String | The fully qualified name of the property (matches the path-based name). |
| `description` | String | A brief explanation of what the property does. |
| `propertyType` | String | The data type of the property (e.g., `Int`, `Bool`, `Float`, `String`, `Struct`). |
| `tags` | Array | A list of strings used for filtering or categorizing properties. |
| `value` | String | The actual value of the property, represented as a string. |

#### Example JSON (`Weapon/SMG/Damage.json`):
```json
{
	"propertyName": "Weapon.SMG.Damage",
	"description": "How much damage the SMG does per bullet.",
	"propertyType": "Int",
	"tags": [],
	"value": "10"
}
```

### Design Philosophy: One File Per Property

LiveConfig intentionally stores each configuration property in a separate file rather than using a single large configuration file. This approach is primarily driven by **Source Control** best practices and is inspired by [OFPA](https://dev.epicgames.com/documentation/en-us/unreal-engine/one-file-per-actor-in-unreal-engine):

1.  **Reducing Merge Conflicts**: When multiple developers or designers change different settings simultaneously, they are editing different files. This avoids multiple people trying to modify the same large JSON/INI file at once.
2.  **Granular History**: Source control systems (like Git or Perforce) can track the history of each individual property. You can easily see who changed a specific value, when, and why, without digging through a massive commit that touched dozens of unrelated settings.
3.  **Easier Reviews**: Pull requests and code reviews become much clearer. If only one value is changed, only one small file appears in the diff.

Additionally, in the case that there _is_ a merge conflict, it can be resolved easily compared to fixing a merge conflict in a binary asset such as a curve table, data table, or blueprint.

### Reference Tracking and Renaming

#### Reference Tracking
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

### Structs

LiveConfig supports C++ and Blueprint structs implicitly.

#### How it Works

The system uses a prefix-based lookup. When you call `GetLiveConfigStruct<FMyStruct>(FLiveConfigProperty("SomeStruct"))`, the system iterates through each `UPROPERTY` defined in `FMyStruct` and attempts to find a corresponding LiveConfig property named `SomeStruct.MemberName`.

#### Supported Member Types

The following property types are supported within a struct:
- `float`, `double`
- `int32`
- `bool`
- `FString`, `FName`, `FText`
- `Enum` (looked up by name string)

#### Example

If you have a struct defined as:
```cpp
USTRUCT(BlueprintType)
struct FMyConfigStruct
{
    GENERATED_BODY()

    UPROPERTY()
    float Speed = 0.0f;

    UPROPERTY()
    int32 Count = 0;
};
```

You can populate it from LiveConfig properties `Player.Settings.Speed` and `Player.Settings.Count` using:
```cpp
FMyConfigStruct Settings = ULiveConfigSystem::Get().GetLiveConfigStruct<FMyConfigStruct>(FLiveConfigProperty("Player.Settings"));
```

For a concrete implementation example, see `FLiveConfigStructLookupTest` in `LiveConfigStructTest.cpp`.

Structs are supported in the `GetLiveConfigValue` blueprint node.

TODO: Adding properties to structs will not cause those subproperties to get initialized in the struct config. Removing properties will result in dangling subproperties.
