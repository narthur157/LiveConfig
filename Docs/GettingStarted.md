# Getting Started

The minimum setup for LiveConfig is the same as for any Unreal plugin - place the `LiveConfig` plugin folder in `<project_root>/Plugins/` and enable the plugin in `Edit -> Plugins` in the editor.

LiveConfig does not have a server component. If you are using source control, make sure that the JSON files being created in `<project_root>/Config/LiveConfig/*.json` are tracked in source control and treated as text files (+w in Perforce).

## Create and use a property

Click the Live Config button to the right of the play button to open the Property Manager.

Click "Add new property" (or press A)

Enter your property name.

In a Blueprint, create a `GetLiveConfig` node. Select your property (note you can also create a property here instead). 

Your value is now ready to use!

## Further setup

### Migrating from Curve Tables

If you are currently using curve tables, you can import the curve tables in project settings. This will create properties for each of your curve table entries.


### Google Sheets

[Sync Google Sheets](./Features/SyncGoogleSheets.md) - Sync data to Google Sheets. This can be done as needed, as a CI job, etc.

## Learn More

[What is Live Config?](./Concepts/WhatIsLiveConfig.md)

[Config Data Structure](./Concepts/ConfigDataStructure.md)