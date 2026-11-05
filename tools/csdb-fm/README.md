# csdb-fm

可视化查看 csdb-teach 二进制格式的 **TUI 工具**：`.dbd` 数据文件 / `.dbi` 索引文件。
解析逻辑与 `inc/record.h`、`inc/index.h` 的 C 布局对应（x86-64 小端）。

## 安装与运行

```bash
cd tools/csdb-fm
npm install
npm run build

# TUI 交互查看
node dist/cli.js ../../build/students.dbd
node dist/cli.js ../../build/students.dbi

# 或安装为全局命令后
npm link
csdb-fm ../../build/students.dbd
```

需要 Node.js >= 18。文件会整体读入内存（100 万行约 280MB），纯查看不写文件。

## TUI 操作

| 按键      | 作用                       |
| --------- | -------------------------- |
| 1 / 2 / 3 / 4 | 切换视图：文件头 / 记录 / 详情 / 布局 |
| ↑ / ↓（或 j / k） | 记录/详情视图中移动光标 |
| PgUp / PgDn | 翻页（11 行）            |
| g / G     | 跳到首行 / 末行            |
| 回车      | 记录列表进入详情           |
| q / Esc   | 退出                       |

4 个视图：

1. **文件头**——逐字段（偏移/大小/值）+ 原始 hex；`.dbi` 会把 `csv_mtime` 换算成日期。
2. **记录**——光标附近 11 行一览（`.dbd` 显示 id/学号/姓名，`.dbi` 显示 id/offset）。
3. **详情**——当前行的全字段解码（含行内偏移）+ 整行原始 hex。
4. **布局**——文件 ASCII 布局图：头 + 首/尾/光标行，标注“你在这里”，适合讲课。

## 非交互模式（脚本 / CI）

```bash
node dist/cli.js --head ../../build/students.dbd     # 只看文件头
node dist/cli.js --row 0 ../../build/students.dbd    # 只看第 0 行
node dist/cli.js --hex 0,64 ../../build/students.dbd # 看 [0,64) 的 hex
node dist/cli.js --dump 5 ../../build/students.dbi   # 转储前 5 条
```

无 TTY 时只能用以上 flags（与 csdb-gd 的 `-y` 思路一致）。

## 开发

```bash
npm run build   # tsc 编译到 dist/
npm test        # 编译 + node --test
```

目录结构：

```
src/
  cli.tsx      # 命令行入口（flags / TUI 启动）
  app.tsx      # Ink TUI（文件头 / 记录 / 详情 / 布局四视图）
  format.ts    # 二进制解析（魔数 / 头 / 行 / 条目 / hex / CSV 转义）
  test/        # node:test 单元测试（内存构造 fixture，不依赖真实大文件）
```
