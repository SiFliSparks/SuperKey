import { navbar } from "vuepress-theme-hope";

export const zhNavbar = navbar([
  "/",
  {
    text: "快速入门",
    icon: "lightbulb",
    link: "/get-started/",
  },
  {
    text: "项目介绍",
    icon: "code",
    link: "/project-introduction/",
  },
  {
    link: "/architecture/",
    text: "架构设计",
    icon: "fa-solid fa-sitemap"
  },
  {
    link: "/custom/",
    text: "自定义",
    icon: "fa-solid fa-wrench",
  },
]);
