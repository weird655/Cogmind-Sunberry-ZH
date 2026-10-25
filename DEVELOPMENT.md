# 构建与适配

Windows、Python 3、LLVM MinGW x86 工具链。标准库即可，无需安装 Python 第三方库。

```powershell
$env:COGMIND_CRT_TOOLCHAIN = 'C:\Tools\llvm-mingw\bin'
python build.py --smoke
python build.py --translations 'C:\Translations\CogmindZH' --tests
python package.py --translations 'C:\Translations\CogmindZH' --identity-extracted 'C:\Audit\structured.tsv' --terminal-native-export 'C:\Audit\terminal-native-records.tsv' --game-data 'C:\Games\Cogmind\data'
```

译表目录需提供 zh_para.tsv、item_names.tsv、zh_trans.tsv、zh_extra.tsv、runtime_patches.tsv。它们不在这个公开源码准备包内。完整语料回归有当前数据的基线数量约束，换用其他语料时应更新预期值，不能直接删去失败断言。

输出在 build/。独立 PhysFS 自动加载器只校验原版 EXE、数据库及六项汉化资源，不读取 Mod 目录，不注册或生成新物品。package.py 会生成 ZIP 安装包；原版 physfs.dll 由安装器在用户本机备份并建立转发文件，不随包分发。旧 Mod Loader 必须先用自己的卸载入口移除。

安装器源码在 installer/，CMD仅负责安装、检查和卸载。游戏直接EXE或Steam启动，不需要另一个启动脚本。

四种入口 profile：显示、终端、维修、名称。统一函数返回 1成功、0未命中、负值错误。底层精确/模板的1/2区分不泄漏到原生适配器。解析先临时写入，成功才提交，失败保持原文和规则编号。

原生对象、链接ID、动画、颜色和生命周期由各入口适配器管理。共享解析不等于任意缓冲区都能写 UTF-8。原文保留，中文仅用于运行时显示；不是重写整个数据库。

中文按两个原生字符格绘制；当前字符注册表、字体扩展和布局并非通用多语言系统。俄文需要额外字形及宽度适配。阿拉伯文需要 BiDi、字形整形、组合字符和混合数字方向处理，不能只替换 TSV。

原生 fixture 必须在独立合法游戏副本中运行，其宏会自动结束测试进程，禁止放进正式发行核心。测试显示成功不等于 Mod 注册、物品生成或真实黑入业务成功。

GitHub 发布准备：保留 LICENSE/NOTICE，上传 assets/language-switch.gif，发布对应安装 ZIP 与版本说明。不要上传游戏程序、数据库、存档、完整未获授权译表及本机运行日志。

安装ZIP仅包含“cogmind汉化261019”文件夹；顶层为安装汉化.cmd、使用说明.txt、资源/。资源平铺保存，无内层ZIP。Install.ps1从所在目录向上定位COGMIND.exe，普通用户无需输入路径。单入口菜单回车安装、2检查、3卸载。

切换动画在 replacement_animation.h 中独立实现：按原坐标覆盖新字，未替换的旧字不重排；CJK 字形保持成对，长度不同时沿两种文本的共同占位范围推进。live_language.h 管理 UI 线程上的状态、原生合成与排队反向切换。无译表烟测包含动画与可逆文字缓存检查。

原生动画取帧宏 ANIMATION_VISUAL_PROBE 需与 CAPTURE_DIAGNOSTICS 一起使用；LIVE_RAPID_PROBE 用于 LIVE_RESOURCE_PROBE / CONTENT_NATIVE_PROBE 的排队反向切换测试。这些测试宏均不得放入正式发行核心。

入口集合、文本对象和本轮迁移范围见 [文本架构](TEXT_ARCHITECTURE.md)。

构建会自动检查文字路由。可单独运行 `python tools/audit_text_routes.py --json route-report.json` 导出入口清单；完整验证还会检查路由守卫本身。


v0.2.2 默认构建已验证的 Steam 260906a 原生地址适配，表在 src/native_layout_steam.h。构建检查每个已登记地址；指定 --game-data 时还校验其上级的 COGMIND.exe 指纹。清单完整不等于通用版本兼容，也不等于所有入口均经过实机验证。

动态事件规则在 data/reviewed-events/；python tools/generate_event_dynamic.py --check 检查占位符、参数类型和固定数值。生成头文件不能代替运行时参数及原生布局测试。原生数值、实体代码和未知参数不做模糊替换。


第47批：原生终端鼠标选择器位于 `src/terminal_picker_*`，父子展开、参数页码与滚动视口分别管理。绘制在临时网格副本上完成，恢复原生状态；重复命令经当前终端与参数集合复核后才单次提交。记忆仅在会话内，不能把一次提交成功解释为黑入结果成功。

`tools/generate_native_ui_robots.py --check` 校验 `data/reviewed-ui/robot-membership.json` 和生成头的一致性；提供 `--catalog` 与 `--game-data` 可重复核对337个原生字段身份。该清单只约束已验证版本的机器人分析参数，不是通用NPC识别模型。

发行 `v0.2.2-r47` 仍使用0.2.2安装版本；新的修订标签与资产区分，旧标签不移动。完整版本边界见docs/LOCALIZATION-AUDIT-20261025.md。
