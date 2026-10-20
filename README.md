# Cogmind 中文 · 太阳莓

**按一下，让另一种语言逐字接管画面。**

<p align="center">
  <a href="assets/language-switch.gif"><img src="assets/language-switch.gif" width="360" alt="Cogmind 局内中英文逐字替换演示"></a>
</p>

**右 Alt＋F9**，局内切换中文与英文。新文字逐个替换旧文字，保留游戏的终端感。

支持已适配的物品名称、物品详情、终端正文、日志、警报与手册等界面。当前适配 **Windows · Cogmind Beta 17.1**。

## 安装

**[下载 v0.2.0 安装包](https://github.com/weird655/Cogmind-Sunberry-ZH/releases/download/v0.2.0/Cogmind-Sunberry-ZH-v0.2.0.zip)** · [版本说明](https://github.com/weird655/Cogmind-Sunberry-ZH/releases/tag/v0.2.0)

适用于 **Windows · Cogmind Beta 17.1**，安装时自动核对版本。一个 ZIP 即可安装，**无需 Mod 前置或旧版补丁**。

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

v0.2.0 加入终端中文输入与候选补全：打开“手动指令”，用拼音选词确认中文，↑↓ 选择候选、Tab 补全，另按 Enter 提交。候选仅来自当前终端已登记的可用命令；图纸、分析等参数类型尚未全部接通，仍可使用英文。

包含 48 条终端正文新增／修正、部件效果与启动提示补全，以及切换后的残留与英文还原修复。已通过离线回归及独立原生进程测试；真实拼音输入与中文指令执行已获用户验证。

部分界面和文本仍有漏译。遇到问题可在 [Issues](https://github.com/weird655/Cogmind-Sunberry-ZH/issues) 附上截图、游戏版本和触发步骤，方便定位。

游戏更新后需要重新核验适配。目前支持中英文切换，其他语言尚未适配。

汉化组：太阳莓。源码公开，限非商业使用；自有框架采用 [CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/)。历史译表来源说明见 [NOTICE](NOTICE.md)。本项目并非游戏官方作品。

<details>
<summary>·</summary>

[实现](src/) · [文本架构](ARCHITECTURE.md) · [构建与适配](DEVELOPMENT.md) · [贡献说明](CONTRIBUTING.md) · [许可](LICENSE)

</details>
