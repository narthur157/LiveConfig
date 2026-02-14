# Live Config Plugin

Create tuning values and feature flags on the fly while coding, access them all from from developer friendly interfaces.

This plugin aims to improve the dev experience of creating and managing game data based on years of experience working on a multiplayer title.

Tuning and feature flags shouldn't be an afterthought buried in binary assets or .ini files. Live Config pulls your tuning data and feature flags away from the implementation.

## Documentation

The full documentation, including the API reference, is available at:
[https://narthur157.github.io/LiveConfig/](https://narthur157.github.io/LiveConfig/)


## Key Features

- **Editor Tooling**:
    - **Live Config Manager**: A dedicated window to manage all your properties, tags, and overrides.
    - **Blueprint Integration**: Special blueprint node for easy access to config properties, provides type-safe access and autocomplete.
    - **CVar per-property**: Automatic autocomplete for live config properties in the console. Easily test editing values while playing in-editor (or in test builds).
  
- **Free to use, modify, and setup** No server required. Open source.
- **Built for source control** Live config uses one JSON file per property, preventing excessive merge conflicts and creating a readable diff for tuning changes.
- **Exportable game database** Easily create a wiki or other knowledge store using your game's actual data
- **Property Tags** Tagging properties provides opportunities not only for organizing, but building additional tooling around properties
- **Google Sheets Integration**: Use a public readable Google Sheet (published as CSV) to override config values at runtime.
- **Type support**: Properties are tied to particular types, including structs.
- **Curve Table Support**: Seamlessly sync remote values with Unreal's native Curve Tables. This is particularly useful for `FScalableFloat` from GAS
- **Profile System**: Create sets of tuning changes and save them to their own file.
- **Network Replication**: Server-authoritative config values are efficiently replicated to clients.

## Comparison with Similar Tools

| Tool | Pros | Cons |
| :--- | :--- | :--- |
| **Live Config** | **Easy Setup**: Up and running in minutes.<br>**Designer Friendly**.<br>**Real-time**: Updates live in-game with polling.<br>**Type-safe**: Strong integration with Unreal types and structs. | Requires internet for remote updates.<br>Not intended for binary data. |
| **Unreal Hotfix Manager** | Native Epic support.<br>Can patch code and assets, not just variables. | Very complex to set up.<br>Requires dedicated backend infrastructure (CDN/Manifests).<br>High friction for simple variable tweaks. |
| **Firestore (Firebase)** | Real-time, highly scalable.<br>Cloud-native features (Auth, Analytics). | Requires Firebase SDK integration.<br>Overkill for simple config.<br>Not natively designed for Unreal's data flow. |
| **PlayFab / AppConfig** | Cloud-native, scalable.<br>Supports complex rules and segments. | Requires third-party SDK.<br>Costs can scale with usage.<br>Less integrated with local Unreal structs. |
| **Normal Config (.ini)** | No internet required.<br>Standard Unreal workflow. | Requires game restart (usually) to apply changes.<br>Hard to manage remotely for live games.<br>No central source of truth for a distributed team. |


## Quickstart Guide

### 1. Basic Setup
1. Copy the `LiveConfig/` folder to your `<project_root>/Plugins/` folder (create `Plugins` if necessary)
1. Enable **Live Config** from the editor in Edit -> Plugins.

### 2. Defining Properties
Option A) From any `GetLiveConfigValue` node, or any `FLiveConfigProperty` type, there is an option to `AddProperty`

Option B)
Open the **Live Config Property Manager** from the **Live Config** button in the Level Editor toolbar (next to the Play button).

From here you can:
- Edit properties
- Add new properties.
- Assign tags (e.g., `Combat`, `Movement`).

### 3. Use properties

#### In C++
```cpp
#include "LiveConfigLib.h"

// Get a float value
float Speed = ULiveConfigLib::GetLiveConfigValue<float>("Hero.MoveSpeed");

// Map a whole struct 
FVector Stats = ULiveConfigLib::GetLiveConfigStruct<FVector>("SomeVector");
```

#### In Blueprint
- **Get Value**: Use the `Get Value` nodes and select your property from the dropdown.
- **Promote to Live Config**: Right-click on any compatible pin in a Blueprint and select "Promote to Live Config" to automatically create a linked property.

# Credit

This plugin was developed with invaluable assistance from Joe Finley https://github.com/jfinley2017
