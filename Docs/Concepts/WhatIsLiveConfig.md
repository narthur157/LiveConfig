# What is Live Config?

Live Config is a static game data management tool for Unreal Engine that is especially well suited to tuning data and feature flags. It exposes this data for remote management, live tuning experiments, and automated database creation. 

It's built for collaboration, visibility, and ease of use.

Live Config at its core allows the creation of JSON backed properties that are accessible anywhere.

### What is tuning data?

Tuning data includes things like damage done by a particular ability, move speed, or any particular value controlling some behavior. This can end up as spreadsheet data (and in turn curve tables).

### What are feature flags?

Feature flags are boolean switches to control whether some part of your application is enabled or not. 

Here is one description of them: https://trunkbaseddevelopment.com/feature-flags/

How exactly they are used is up to your project's specific needs but here are some general categories of feature flags:

- **Content flags** - Managing whether something like a particular map, character, ability, etc. is enabled or not. Paired with remote overrides, content flags can prevent something like an isolated crash from rendering the whole game unplayable.
- **System flags** - When working on new systems, including a feature flag for the entire system can help to perform speculative tests without overcommitting. These flags can also be used while debugging to rule out entire systems as part of the process.
- **Access flags** - Control buttons on your main menu to gate access to features such as matchmaking, lobbies, tutorials, etc.

Live Config supports deprecation of any property which allows for flags to be gradually removed. This is essential for flags in particular, which can be difficult to remove confidently.

## Collaboration + Visibility

Each property is its own JSON file in order to reduce merge conflicts and create a source control history of changes to each property. 

When looking at a set of changes, it is easy to see what new tuning values were created or modified. 

Because the Live Config data can be exported easily, this can help to make data visibility to people without access to the editor.

Setting up properties distances data from implementation, which makes it easier to think about tuning changes without needing to think (or know) about how something was built.