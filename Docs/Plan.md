Support metadata tags in the property ui directly

Tags should be helpful for organizing properties, and constructing ad-hoc types

Getting all properties with a given tag ought to be useful for some more specific editor UI's


## Property Manager UI
Similar in concept to the gameplay tag manager, but with a more prominent menu entry near the play button. Should support CRUD operations on properties, and their tags. You should also be able to edit values here

The property customization UI should have a way to "open property in manager" to customize its tags/values in a more full UI setup

UX tricks for converting a project to use live config
- Add a menu entry next to "promote to variable". This should auto-fill as much as possible, eg the property name, a tag, the type, and value
- Curve table integration


## Curve table integration
- Add existing curve tables via project settings and property manager UI. Option to add a prefix + tags to curve table values. These tables have their values registered as properties. Any properties coming from a table should be auto tagged with that table's name
- Default curve table that all properties are added to automatically. Only values that are not in another table are added to this one. This table
- Warning for duplicate curve table values
- Auto add all float properties to curve tables for GE support with implicit conversion of .1, .2, .3 etc. properties
- Option to auto-discover and register curve tables. On by default

GetArray support using the .1, .2, .3 etc. implicit syntax

## CSV Export

CSV export is a way to setup the project with sheets. It exports the property name, value, tags, and type as columns.