/**
 * @mainpage Live Config API Reference
 *
 * @section intro_sec Introduction
 *
 * Welcome to the Live Config API documentation. This plugin provides a robust, type-safe system for 
 * managing game configuration and tuning values with real-time updates and source control integration.
 *
 * @section core_api Core API Access Points
 *
 * If you are looking to integrate Live Config into your C++ or Blueprint code, these are the primary classes and structs you should start with:
 *
 * - @ref ULiveConfigSystem "ULiveConfigSystem": The central engine subsystem for accessing configuration values. Use `ULiveConfigSystem::Get()` to access the singleton instance.
 * - @ref FLiveConfigProperty "FLiveConfigProperty": A type-safe wrapper for property names. It handles redirects and provides a clean interface for specifying which property you want to access.
 * - @ref ULiveConfigLib "ULiveConfigLib": A Blueprint function library providing easy access to Live Config values from Blueprints.
 * - @ref ULiveConfigProfileSystem "ULiveConfigProfileSystem": Manages tuning profiles, allowing you to switch between different sets of configuration values at runtime.
 *
 * @section quick_examples Quick C++ Examples
 *
 * @subsection get_value Getting a Simple Value
 * @code{.cpp}
 * #include "LiveConfigSystem.h"
 *
 * float MoveSpeed = ULiveConfigSystem::Get().GetFloatValue("Hero.MoveSpeed"_LC);
 * @endcode
 *
 * @subsection get_struct Getting a Struct
 * @code{.cpp}
 * FMyHeroStats Stats = ULiveConfigSystem::Get().GetLiveConfigStruct<FMyHeroStats>("Hero.Stats"_LC);
 * @endcode
 *
 * @section navigation Navigation
 *
 * - Use the **Classes** tab to see a list of all available classes and structs.
 * - Use the **Files** tab to explore the source code structure.
 * - The **Class Index** provides an alphabetical list of all types.
 */
