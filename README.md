# Live Config Plugin

![UE Version](https://img.shields.io/badge/Unreal%20Engine-5.7-0e1128?logo=unrealengine&logoColor=white)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/narthur157/LiveConfig/deploy-docs.yml?label=docs)
![Bluesky followers](https://img.shields.io/bluesky/followers/nickarthurdev.bsky.social)



Live Config is a tool to manage game data in Unreal Engine.

![Node screenshot](./Docs/Screenshots/LiveConfig-GetValueNodes.png)

![Property Manager screenshot](./Docs/Screenshots/LiveConfig-PropertyManager.png)

Define and and retrieve JSON backed properties that can be accessed from anywhere. Supported by tooling for organization, tracking references, and remote or session-based overrides.

## Documentation

The full documentation, including the API reference, is available at:
[https://narthur157.github.io/LiveConfig/](https://narthur157.github.io/LiveConfig/)


## Key Features

- **Editor Tooling**:
    - **Live Config Manager**: A dedicated window to manage all your properties, tags, and overrides.
    - **Blueprint Integration**: K2 node for easy access to config properties, provides type-safe access and autocomplete.
    - **CVar per-property**: Console autocomplete for live config property overrides. Test changes without restarting the play session.
  
- **Free** No server needed, MIT Open source.
- **Built for source control** Live config uses one JSON file per property, preventing excessive merge conflicts and creating a readable diff for tuning changes.
- **Exportable game database** Easily create a wiki or other knowledge store using your game's actual data
- **Property Tags** Tagging properties provides opportunities not only for organizing, but building additional tooling around properties
- **Google Sheets Integration**: Use a Google Sheet (or any remote CSV source) to remote override config values at runtime.
- **Type support**: Properties are tied to particular types, including structs.
- **Curve Table Support**: Seamlessly sync remote values with Unreal's native Curve Tables. This is particularly useful for `FScalableFloat` from GAS
- **Profile System**: Create sets of tuning changes and save them to their own file.
- **Network Replication**: Server-authoritative config values are efficiently replicated to clients.

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
