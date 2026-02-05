# Live Config Plugin

Centralize your game data

Use feature flags to deploy confidently and differentiate environments

Skip digging through blueprints, curve tables, data tables for tuning values

Make tuning changes in realtime, in any build

## Why use Live Config?

Game balancing and iteration requires endless tiny tweaks to variables. Unreal lacks tools to efficiently manage these variables during development and live operations. 

Live Config bridges the gap by providing a designer-friendly workflow while maintaining high performance through local caching and type-safe access in C++ and Blueprints. It is particularly useful during development for rapid iteration and in live-service games for "over-the-air" balancing.



## Key Features

- **Editor Tooling**:
    - **Live Config Manager**: A dedicated window to manage all your properties, tags, and overrides.
    - **Blueprint Integration**: Special blueprint node for easy access to config properties, provides type-safe access and autocomplete.
    - **CVar per-property**: Automatic autocomplete for live config properties in the console. Easily test editing values
  
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
| **Live Config** | **Easy Setup**: Up and running in minutes.<br>**Designer Friendly**.<br>**Real-time**: Updates live in-game with polling.<br>**Type-safe**: Strong integration with Unreal types and structs. | Requires internet for remote updates.<br>Sheet must be public (or accessible via URL).<br>Not intended for massive binary data. |
| **Unreal Hotfix Manager** | Native Epic support.<br>Can patch code and assets, not just variables. | Very complex to set up.<br>Requires dedicated backend infrastructure (CDN/Manifests).<br>High friction for simple variable tweaks. |
| **Firestore (Firebase)** | Real-time, highly scalable.<br>Cloud-native features (Auth, Analytics). | Requires Firebase SDK integration.<br>Overkill for simple config.<br>Not natively designed for Unreal's data flow. |
| **PlayFab / AppConfig** | Cloud-native, scalable.<br>Supports complex rules and segments. | Requires third-party SDK.<br>Costs can scale with usage.<br>Less integrated with local Unreal structs. |
| **Normal Config (.ini)** | No internet required.<br>Standard Unreal workflow. | Requires game restart (usually) to apply changes.<br>Hard to manage remotely for live games.<br>No central source of truth for a distributed team. |

## Quickstart Guide

### 1. Basic Setup
1. Enable the **Live Config** plugin in your project.
2. In the Unreal Editor, go to **Project Settings** -> **Project** -> **Live Config**.
3. Set your **Sheet URL**. This should be a link to a Google Sheet published as CSV.
   - *Google Sheets: File -> Share -> Publish to web -> Select Sheet and 'Comma-separated values (.csv)' -> Copy Link.*

### 2. Defining Properties
Open the **Live Config Property Manager** from the **Live Config** button in the Level Editor toolbar (next to the Play button).

From here you can:
- Add new properties.
- Assign tags (e.g., `Combat`, `Movement`).
- Set default values and types.
- Import/Export from Curve Tables.

### 3. Using in Code (C++)
```cpp
#include "LiveConfigSystem.h"

// Get a float value
float Speed = ULiveConfigSystem::Get().GetFloatValue("Hero.MoveSpeed");

// Map a whole struct (automatically looks up Hero.Stats.Health, Hero.Stats.Armor, etc.)
FMyHeroStats Stats = ULiveConfigSystem::Get().GetLiveConfigStruct<FMyHeroStats>("Hero.Stats");
```

### 4. Using in Blueprints
- **Get Value**: Use the `Get Value` nodes and select your property from the dropdown.
- **Promote to Live Config**: Right-click on any compatible pin in a Blueprint and select "Promote to Live Config" to automatically create a linked property.

# Credit

This plugin was developed by Nick Arthur with invaluable assistance and advice from Joe Finley. Additionally, I'd like to credit the team at Genpop for using the plugin which inspired this one.