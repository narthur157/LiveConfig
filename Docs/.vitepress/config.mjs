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
          { text: 'Getting Started', link: '/ConfigDataStructure' },
          { text: 'Download', link: 'https://github.com/narthur157/LiveConfig/releases' },
        ]
      },
      {
        text: 'Features',
        items: [
          { text: 'Curve Tables', link: '/Features/CurveTables' },
          { text: 'Config Profiles', link: '/Features/Profiles'},
          { text: 'Sync to Google Sheets', link: '/Features/SyncGoogleSheets' },
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
