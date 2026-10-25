# Cogmind 中文 · 太阳莓

**按一下，让另一种语言逐字接管画面。**

<p align="center">
  <a href="assets/language-switch.gif"><img src="assets/language-switch.gif" width="360" alt="Cogmind 局内中英文逐字替换演示"></a>
</p>

**右 Alt＋F9**，局内切换中文与英文。新文字逐个替换旧文字，保留游戏的终端感。

支持已适配的物品名称、物品详情、终端正文、日志、警报与手册等界面。当前适配 **Windows · Cogmind Beta 17.1**。

## 安装

**[下载 v0.2.2 安装包（第47批）](https://github.com/weird655/Cogmind-Sunberry-ZH/releases/download/v0.2.2-r47/Cogmind-Sunberry-ZH-v0.2.2-r47.zip)** · [版本说明](https://github.com/weird655/Cogmind-Sunberry-ZH/releases/tag/v0.2.2-r47)

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

**v0.2.2 · 第47批修订** 已同步后续汉化与终端界面改进。安装器内版本号仍为 0.2.2，发行标签为 `v0.2.2-r47`。

终端指令可以直接用鼠标：点“手动指令”，展开分类或目标，选好后点“执行指令”。界面使用游戏的原生字体和网格，长列表在固定框内滚动，底部按钮保持可见。“重复上次命令”会重新检查当前终端和目标；“记忆”保留本次游戏里的展开、选择、页码和滚动位置。仍可切回键盘输入，结果和成功率由原版判定。

第47批补齐、修正及润色 **95 项文本／句式**：35 条通讯和对白、12 项装备效果、12 个固定指令显示别名、5 类动态消息、31 个完整界面标题。重点补上“标签已中文、正文仍英文”的装备说明，并纠正条件、数量、热量、后坐力及叠加上限；另有 1 个未证可达标题兼容键，不计入上述 95 项。

当前包只支持已验证的 **Steam Windows Beta 17.1（260906a）**，保留原版 EXE 并严格检查指纹。旧版先卸载再安装，保留当前存档进度。

28 组 C 回归及配套 Python 检查、21,958 条静态文本重放、612 次隔离原生重绘通过；原有 9,870 条成功命中未丢失。详见 [更新记录](CHANGELOG.md)、[本批验证](VALIDATION.md) 和 [核查范围与剩余问题](docs/LOCALIZATION-AUDIT-20261025.md)。这些结果不代表全游戏剧情与终端已逐一实测。

部分界面和文本仍有漏译。遇到问题可在 [Issues](https://github.com/weird655/Cogmind-Sunberry-ZH/issues) 附上截图、游戏版本和触发步骤，方便定位。

游戏更新后需要重新核验适配。目前支持中英文切换，其他语言尚未适配。

汉化组：太阳莓。源码公开，限非商业使用；自有框架采用 [CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/)。历史译表来源说明见 [NOTICE](NOTICE.md)。本项目并非游戏官方作品。

<details>
<summary>·</summary>

[实现](src/) · [文本架构](ARCHITECTURE.md) · [构建与适配](DEVELOPMENT.md) · [贡献说明](CONTRIBUTING.md) · [许可](LICENSE)

</details>
