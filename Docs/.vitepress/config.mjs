import { defineConfig } from 'vitepress'

// https://vitepress.dev/reference/site-config
export default defineConfig({
  title: "Live Config",
  description: "Live Config Docs",
  base: '/LiveConfig/',
  appearance: 'dark',
  cleanUrls: true,
  themeConfig: {
    // https://vitepress.dev/reference/default-theme-config
    nav: [
      { text: 'Home', link: '/' },
    ],

    sidebar: [
      {
        text: 'Introduction',
        items: [
          { text: 'Getting Started', link: '/GettingStarted' },
          { text: 'Download', link: 'https://github.com/narthur157/LiveConfig/releases' },
        ]
      },
      {
        text: 'Concepts',
        items: [
          { text: 'About', link: '/Concepts/WhatIsLiveConfig' },
          { text: 'Config Data Structure', link: '/Concepts/ConfigDataStructure' }
        ]
      },
      {
        text: 'Features',
        items: [
          { text: 'Remote Overrides', link: '/Features/RemoteOverrides' },
          { text: 'Curve Tables', link: '/Features/CurveTables' },
          { text: 'Config Profiles', link: '/Features/Profiles'},
          { text: 'Sync to Google Sheets', link: '/Features/SyncGoogleSheets' },
          { text: 'Reference Tracking', link: '/Features/ReferenceTracking' }
        ]
      },
      {
        text: 'API Reference',
        link: '/api/index.html',
        target: '_blank'
      }
    ],

    socialLinks: [
      { icon: 'github', link: 'https://github.com/narthur157/LiveConfig' }
    ]
  }
})
