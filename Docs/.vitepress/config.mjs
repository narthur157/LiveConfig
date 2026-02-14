import { defineConfig } from 'vitepress'

// https://vitepress.dev/reference/site-config
export default defineConfig({
  title: "Live Config Plugin",
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
          { text: 'CI Setup', link: '/CISetup' },
        ]
      },
      {
        text: 'API Reference',
        link: '/api/index.html',
        target: '_blank'
      }
    ],

    socialLinks: [
      { icon: 'github', link: 'https://github.com/vuejs/vitepress' }
    ]
  }
})
