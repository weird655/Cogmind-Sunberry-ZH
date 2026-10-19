# Cogmind 中文 · 太阳莓

**按一下，让另一种语言逐字接管画面。**


<p align="center">
  <a href="assets/language-switch.gif"><img src="assets/language-switch.gif" width="420" alt="Cogmind 局内中英文逐字替换演示"></a>
</p>

**右 Alt＋F9**，局内切换中文与英文。新文字逐个替换旧文字，保留游戏的终端感。

支持已适配的物品名称、物品详情、终端正文、日志、警报与手册等界面。当前适配 **Windows · Cogmind Beta 17.1**。

## 安装

**[下载 v0.1.0 安装包](https://github.com/weird655/Cogmind-Sunberry-ZH/releases/download/v0.1.0/Cogmind-Sunberry-ZH-v0.1.0-update.zip)** · [版本说明](https://github.com/weird655/Cogmind-Sunberry-ZH/releases/tag/v0.1.0)

这是首个公开版本。**当前附件为增量更新，需要已安装第三轮统一版；不能直接安装在纯净原版上。** 安装器会核对基础文件，不匹配时停止安装。

1. 下载 ZIP，并将整个压缩包解压到一个文件夹。
2. 在游戏中正常保存退出。
3. 双击解压后的 `安装修复.cmd`，填写含 `COGMIND.exe` 的游戏目录。
4. 提示安装完成后，照常通过游戏 EXE 或 Steam 启动。
5. 局内按 **右 Alt＋F9** 切换中英文。

安装前会备份当前存档。CMD 用于安装、检查和还原，不是游戏启动器；不要在压缩包内直接运行。

检查安装：运行 `检查修复.cmd`。撤销此次更新：保存退出后运行 `还原修复.cmd`，不会覆盖之后的游戏进度。

已通过十二套离线回归与原生显示测试。仍可能存在未接通的界面或漏译，欢迎附截图、游戏版本和触发步骤反馈。

游戏更新后需要核验适配；目前只支持中英文切换。俄文尚需扩展字库和宽度规则，阿拉伯文还需双向排版与字形连接处理。

汉化组：太阳莓。源码公开，限非商业使用；自有框架采用 [CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/)。历史译表来源说明见 [NOTICE](NOTICE.md)。本项目并非游戏官方作品。

<details>
<summary>·</summary>

[实现](src/) · [构建与适配](DEVELOPMENT.md) · [许可](LICENSE)

</details>
