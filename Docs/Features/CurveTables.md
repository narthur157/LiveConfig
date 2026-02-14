Curve tables are supported primarily for 2 purposes

### Expose Live Config data to `GameplayEffect`
Gameplay Effects use `FScalableFloat` to provide data. While it is possible to provide live config data as set by caller values, this can be inconvenient.


### Import existing data
If your project currently uses curve tables, you can easily import that data into the live config. This will tag that data as coming from a curve table.

# How-to

In project settings, specify a curve table to export properties to, and/or curve tables to import properties from: ![CurveTableSettings](../Images/ProjectSettings-CurveTables.png)

Properties that are exported or imported will be updated automatically. When live config data is changed, it will be updated in the export curve table.

TODO: Turn import tables into a single action rather than a property