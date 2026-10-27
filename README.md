# 简单文本编辑器

开发者：龙智森
学号：2024414290224

使用本机 Qt 6.10.3 MinGW 64 位和 Qt Widgets 开发，采用 qmake 工程。

## 打开项目

双击本目录的 **SimpleTextEditor.pro**，在 Qt Creator 中选择 **Desktop Qt 6.10.3 MinGW 64-bit** 套件，然后构建并运行。

配置项目时启用 Shadow build，把构建目录设为：
```
F:\Codex\huancun\text-editor-qtcreator
```

mainwindow.ui、searchdialog.ui、aboutdialog.ui 均可在 Qt Designer 中编辑。
主窗口菜单、工具栏和 Action 在 mainwindow.ui 中定义；CodeEditor 通过提升 QPlainTextEdit 使用。
查找和替换共用 searchdialog.ui，根据入口显示或隐藏替换相关控件。

## 功能

- 文件新建、打开、保存、另存为；未保存修改在新建、打开和关闭前提示保存。
- 保存成功后更新文件名和修改标记；写入失败保留原文档状态。
- 剪切、复制、粘贴、撤销、重做、全选；Action 使能状态跟随选区、剪贴板和撤销历史变化。
- 查找和替换：向上/向下、循环查找、区分大小写、全字匹配、单个替换、全部替换及结果提示。
- 全部替换可一次撤销；替换词包含查找词、替换为空文本、没有匹配项等情况均已测试。
- 字体和字号、字体颜色、编辑器背景色、自动换行。
- 行号显示/隐藏，字体或行数变化时自动调整行号栏宽度。
- 工具栏/状态栏显示与隐藏，状态栏显示长度、行数和光标位置。
- 关于窗口显示开发者、学号和当前 Qt 版本。
- 本次范围不包含选做的定时自动保存。

本程序读写 **UTF-8 纯文本**，可打开带 UTF-8 BOM 的文件；保存为 UTF-8。
不支持的编码或包含空字符的二进制内容会提示并拒绝载入，避免乱码覆盖原文件。
字体、字号和颜色是编辑器显示设置，不写入纯文本文件。

常用快捷键：Ctrl+N/O/S 新建/打开/保存，Ctrl+Shift+S 另存为；
Ctrl+Z / Ctrl+Shift+Z 撤销/重做，Ctrl+F/H 查找/替换，Ctrl+L 切换行号。

## 构建与验证

在本目录用 PowerShell 运行：
```powershell
.\scripts\build.ps1 -Test
```

脚本使用已安装的 Qt 和 MinGW，不安装额外软件。
构建产物、临时数据、测试日志和界面截图均位于：
```
F:\Codex\huancun\text-editor-20261027
```

结果文件：test-results.txt。界面检查图位于 test-data/screenshots。
测试源代码位于 tests/editor_tests.cpp，可重复运行上述命令重新生成结果。
Qt Test 的最终统计为 **24 passed, 0 failed**（包含初始化与清理）。

覆盖 UTF-8/中文文件、空文件、文件不存在、无效和截断的 UTF-8、
二进制输入、保存失败、另存为取消、未保存关闭的不同选择、剪贴板状态、
查找与替换边界、行号与换行、字体及颜色对话框、开发者信息等场景。
测试使用临时文件，结束时删除数据文件，并恢复开始前的剪贴板格式和内容。
Windows 剪贴板若被其他程序短暂占用，测试会等待重试。

GitHub 仓库：[Hui-size/Simple-text-editor](https://github.com/Hui-size/Simple-text-editor)。
