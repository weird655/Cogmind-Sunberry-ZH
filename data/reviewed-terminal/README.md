# 自有终端补译

records.json 保存自行编写的译文、原生记录序号、字段位置、字节前缀偏移和原文 SHA-256。未附带完整英文正文或历史第三方译表。

使用 tools/build_reviewed_terminal.py，结合本地已验证的 terminal_results.tsv 导出生成补充 TSV。工具逐条验证记录身份和原文摘要，任一不符即停止；不可用于其他版本的导出。生成结果需合并进自身有权使用的译表，并重新构建 DLL 和加载器哈希。不要直接修改已安装 TSV 后绕过完整性校验。

interface-supplement.json 是额外界面规则。命令汉化仅用于显示，原生参数与判定保持不变。完整框架与文档许可见仓库 LICENSE；第三方内容范围见 NOTICE.md。
