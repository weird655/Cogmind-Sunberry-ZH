# 构建与适配

Windows、Python 3、LLVM MinGW x86 工具链。标准库即可，无需安装 Python 第三方库。

```powershell
$env:COGMIND_CRT_TOOLCHAIN = 'C:\Tools\llvm-mingw\bin'
python build.py --smoke
python build.py --translations 'C:\Translations\CogmindZH' --tests
```

译表目录需提供 zh_para.tsv、item_names.tsv、zh_trans.tsv、zh_extra.tsv、runtime_patches.tsv。它们不在这个公开源码准备包内。完整语料回归有当前数据的基线数量约束，换用其他语料时应更新预期值，不能直接删去失败断言。

输出在 build/。build.py 会同步汉化核心、五份数据的六项校验，生成共享加载器。安装目录的 mods/catalog.ini 也必须同步 SHA0～SHA5；请使用匹配版本的安装脚本，不能只手动覆盖一个 DLL。加载器严格校验已适配 EXE 与游戏数据，不应为支持新版本直接关闭校验。

四种入口 profile：显示、终端、维修、名称。统一函数返回 1成功、0未命中、负值错误。底层精确/模板的1/2区分不泄漏到原生适配器。解析先临时写入，成功才提交，失败保持原文和规则编号。

原生对象、链接ID、动画、颜色和生命周期由各入口适配器管理。共享解析不等于任意缓冲区都能写 UTF-8。原文保留，中文仅用于运行时显示；不是重写整个数据库。

中文按两个原生字符格绘制；当前字符注册表、字体扩展和布局并非通用多语言系统。俄文需要额外字形及宽度适配。阿拉伯文需要 BiDi、字形整形、组合字符和混合数字方向处理，不能只替换 TSV。

原生 fixture 必须在独立合法游戏副本中运行，其宏会自动结束测试进程，禁止放进正式发行核心。测试显示成功不等于 Mod 注册、物品生成或真实黑入业务成功。

GitHub 发布准备：保留 LICENSE/NOTICE，上传 assets/language-switch.gif，发布对应安装 ZIP 与版本说明。不要上传游戏程序、数据库、存档、完整未获授权译表及本机运行日志。
