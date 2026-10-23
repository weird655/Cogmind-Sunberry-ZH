# Cogmind 中文 · 太阳莓

**按一下，让另一种语言逐字接管画面。**

<p align="center">
  <a href="assets/language-switch.gif"><img src="assets/language-switch.gif" width="360" alt="Cogmind 局内中英文逐字替换演示"></a>
</p>

**右 Alt＋F9**，局内切换中文与英文。新文字逐个替换旧文字，保留游戏的终端感。

支持已适配的物品名称、物品详情、终端正文、日志、警报与手册等界面。当前适配 **Windows · Cogmind Beta 17.1**。

## 安装

**[下载 v0.2.2 安装包](https://github.com/weird655/Cogmind-Sunberry-ZH/releases/download/v0.2.2/Cogmind-Sunberry-ZH-v0.2.2.zip)** · [版本说明](https://github.com/weird655/Cogmind-Sunberry-ZH/releases/tag/v0.2.2)

适用于 **Windows · Steam Cogmind Beta 17.1（260906a，已核验指纹）**，安装时自动核对版本。其他 EXE 版本仍需单独适配。一个 ZIP 即可安装，**无需 Mod 前置或旧版补丁**。

1. 下载并解压，把里面的 **“cogmind汉化261019”文件夹放到游戏目录**，和 `COGMIND.exe` 放在一起。
2. 保存并退出游戏。
3. 打开“cogmind汉化261019”文件夹，双击 **`安装汉化.cmd`**，按回车。
4. 提示装好后，照常打开游戏。**右 Alt＋F9** 切换中英文。

安装后照常通过游戏 EXE 或 Steam 启动。CMD 只负责安装、检查和卸载。

检查或卸载：再打开 `安装汉化.cmd`，输入 **2** 检查，输入 **3** 卸载。

更新旧版：先用旧安装入口输入 **3** 卸载，再安装新版。安装会备份存档，卸载保留当前进度。

请先完整解压再安装；“资源”文件夹保持原样即可。

以前装过其他汉化或 Mod 前置的，请先用原来的卸载入口恢复原版。不要把含 Mod 新物品的存档直接迁回原版。

## 当前状态

**v0.2.2** 补充 NPC 对话、动态事件、无人机和投射物等部件效果；修复带回合号的拾取日志漏译，保留数量、重复标记和原生命中计算。

本轮 118 个事件候选已逐项处理：103 项翻译或动态规则，15 项按原生转发、代码或损坏文本保留。终端中文输入与候选补全继续可用：拼音选词上屏，↑↓ 选择、Tab 补全，另按 Enter 提交；部分图纸／分析参数仍需英文。

当前安装包使用已验证的 Steam 260906a 适配。**不会替换游戏 EXE，也不会自动适配所有 Beta 17.1 文件。** 旧版先从原安装入口卸载，再装新版，保留当前存档进度。

20 组 C 回归、18 项 Python 检查、470 个事件样例及 1,880 次宽度检查通过；独立原生进程 300 次重绘无字形错误。详见 [v0.2.2 更新记录](CHANGELOG.md) 和 [核查范围与已知遗漏](docs/LOCALIZATION-AUDIT-20261023.md)。统计不代表全游戏汉化完成。

部分界面和文本仍有漏译。遇到问题可在 [Issues](https://github.com/weird655/Cogmind-Sunberry-ZH/issues) 附上截图、游戏版本和触发步骤，方便定位。

游戏更新后需要重新核验适配。目前支持中英文切换，其他语言尚未适配。

汉化组：太阳莓。源码公开，限非商业使用；自有框架采用 [CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/)。历史译表来源说明见 [NOTICE](NOTICE.md)。本项目并非游戏官方作品。

<details>
<summary>·</summary>

[实现](src/) · [文本架构](ARCHITECTURE.md) · [构建与适配](DEVELOPMENT.md) · [贡献说明](CONTRIBUTING.md) · [许可](LICENSE)

</details>
