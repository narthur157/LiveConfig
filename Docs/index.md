---
# https://vitepress.dev/reference/default-theme-home-page
layout: home

hero:
  name: "Live Config"
  text: "Game data Management for Unreal Engine"
  tagline: Decouple configuration from implementation
  actions:
    - theme: brand
      text: Get Started
      link: /ConfigDataStructure
    - theme: alt
      text: Download
      link: https://github.com/narthur157/LiveConfig
  image: 
    src: /Screenshots/LiveConfig-GetValueNodes.png

features:
  - title: Feature Flags
    details: Use feature flags to deploy changes with confidence. Ship changes, enable them when you're ready. Control features on different environments.
    icon: 🏴‍☠️
  - title: Rapid Iteration 
    details: Tune values via editor UI, Google Sheets, or console commands. 
    icon: 🚤
  - title: Source Control Integrated
    details: Every tuning change is tracked in source control with readable diffs.
    icon: 🗒️
  - title: JSON Config Format
    details: Human readable config file format. 
    icon: 🗒️
  - title: Type-Safe access
    details: Strong integration with Unreal types and structs in both C++ and Blueprints with full autocomplete support.
    icon: 🛡️
  - title: Network Replicated Profiles
    details: Apply per-session tuning profiles for specific scenarios or live tuning.
    icon: 🌐
---

