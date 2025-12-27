import { defineUserConfig } from "vuepress";

import theme from "./theme.js";

export default defineUserConfig({
  base: "/projects/superkey/",

  locales: {
    "/en/": {
      lang: "en-US",
      title: "SuperKey Encyclopedia",
      description: "SuperKey Encyclopedia based on SF32",
    },
    "/": {
      lang: "zh-CN",
      title: "SuperKey百科全书",
      description: "SuperKey百科全书",
    },
  },

  theme,

  head: [
    // 百度统计
    [
      'script',
      {},
      `
      var _hmt = _hmt || [];
      (function() {
        var hm = document.createElement("script");
        hm.src = "https://hm.baidu.com/hm.js?b12a52eecef6bedee8b8e2d510346a6e";
        var s = document.getElementsByTagName("script")[0]; 
        s.parentNode.insertBefore(hm, s);
      })();
      `
    ],
    // 不蒜子统计
    [
      'script',
      {
        async: true,
        src: '//busuanzi.ibruce.info/busuanzi/2.3/busuanzi.pure.mini.js'
      }
    ]
  ],
  // Enable it with pwa
  // shouldPrefetch: false,
});