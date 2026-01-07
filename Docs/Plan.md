## Console command / cvar-like integration
- Add a console command to set properties by name and value. eg `liveconfig set MyProperty 10`

## Config replication
Create an actor that replicates the config from server to clients
If console commands are used to change values, they should be replicated (optionally)


## Curve table integration
- Add existing curve tables via project settings and property manager UI. Option to add a prefix + tags to curve table values. These tables have their values registered as properties. Any properties coming from a table should be auto tagged with that table's name
- Default curve table that all properties are added to automatically. Only values that are not in another table are added to this one. This table
- Warning for duplicate curve table values
- Auto add all float properties to curve tables for GE support with implicit conversion of .1, .2, .3 etc. properties
- Option to auto-discover and register curve tables. On by default

GetArray support using the .1, .2, .3 etc. implicit syntax

## CSV Export

CSV export is a way to setup the project with sheets. It exports the property name, value, tags, and type as columns.