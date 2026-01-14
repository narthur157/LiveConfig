## Config replication
Create an actor that replicates the config from server to clients
If console commands are used to change values, they should be replicated (optionally)
Need to add player controller component so that players have an owning connection for the RPC

## CSV Export

CSV export is a way to setup the project with sheets. It exports the property name, value, tags, and type as columns.

## Struct support
Allow creating properties in groups, so that if I want to create the "Attributes" values for a new hero for example, I would not need to go and add each property 

Major decision: First class support, or implicit support?

First class: Structs would actually get saved out to their own json files, rather than getting accumulated into a number of different files for each individual struct property
Types dropdown in property manager would include any supported struct

Implicit support:
Lots of management to do when adding/removing properties from structs
  - Adding properties: All existing values get default initialized
  - Removing properties: All existing values get removed
  - Renaming properties: All existing values get renamed
  - Changing property type: All existing values get converted to the new type

There are various levels between these, eg we could shove the struct properties into 1 json file for less file bloat, but keep the properties as individual values in the TMaps.

Lots of little decisions to make within that
- When adding or remove properties from the struct, what happens to the data?
  - Adding properties: All existing values get default initialized
  - 