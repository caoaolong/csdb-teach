# csdb-gd

使用 [faker.js](https://fakerjs.dev/) 生成模拟数据并保存为 **CSV** 的小工具，面向数据库课程教学（如学生表、用户表、订单表等示例数据）。

生成时提供 **TUI（终端交互界面）**，可逐步选择：预设模板、包含哪些字段、生成行数、**保存位置**、分隔符、表头、语言、编码、随机种子。

## 安装与运行

```bash
cd tools/csdb-gd
npm install
npm run build

# 交互模式（TUI）
node dist/cli.js
# 或安装为全局命令后
npm link
csdb-gd
```

需要 Node.js >= 18。

## TUI 操作

| 按键      | 作用                     |
| --------- | ------------------------ |
| ↑ / ↓     | 移动光标                 |
| 空格      | 勾选 / 取消字段          |
| a / n     | 全选 / 全不选（字段页）  |
| 回车      | 确认当前步骤             |
| Esc       | 返回上一步               |

配置流程：预设模板 → 字段多选 → 行数 → **保存位置** → 分隔符 → 表头 → 语言 → 编码 → 种子 → 确认生成。

- **保存位置**：输入 `.csv` 文件路径即可，不存在的目录会自动创建。默认 `./csdb-data-YYYYMMDD-HHmmss.csv`。
- **编码**：默认 `UTF-8 with BOM`，Excel 直接打开中文不乱码；通用场景可选纯 `UTF-8`。
- **种子**：留空则每次随机；填入相同种子可复现相同数据（方便留作业 / 对答案）。

## 非交互模式（脚本 / CI）

```bash
# 用学生预设生成 1000 行（默认预设 student）
node dist/cli.js --yes --rows 1000 --out ./data/students.csv

# 指定字段
node dist/cli.js --yes --fields id,name,gender,major,score --rows 500 --out ./data/scores.csv

# 英文数据 + 分号分隔 + 不写表头 + 固定种子
node dist/cli.js --yes --preset user --locale en --delimiter semicolon --no-header \
  --seed csdb-2026 --rows 200 --out ./data/users.csv
```

| 参数 | 说明 |
| ---- | ---- |
| `--preset <name>` | 预设模板：`student` / `user` / `employee` / `order` / `custom` |
| `--fields <list>` | 字段列表（逗号分隔），优先级高于 `--preset` |
| `--rows <n>` | 行数，1~1000000，默认 100 |
| `--out <path>` | 保存位置（`.csv`），默认 `./csdb-data-<时间戳>.csv` |
| `--delimiter <d>` | `comma` / `semicolon` / `tab`，默认 `comma` |
| `--no-header` | 不写表头行（默认写） |
| `--locale <l>` | `zh_CN` / `en`，默认 `zh_CN` |
| `--encoding <e>` | `utf8` / `utf8-bom`，默认 `utf8-bom` |
| `--seed <s>` | 随机种子 |
| `-y, --yes` | 跳过 TUI 直接生成 |
| `--list-fields` / `--list-presets` | 列出可用字段 / 预设后退出 |

TUI 模式下这些参数同样可用，作用是**预填默认值**（如 `csdb-gd --preset order --rows 500` 进入 TUI 时已选好订单字段与行数）。

## 可用字段（`--list-fields`）

`id` 自增ID、`uuid`、`studentNo` 学号、`name` 姓名、`gender` 性别、`age` 年龄、
`birthdate` 出生日期、`major` 专业、`grade` 年级、`course` 课程、`score` 成绩、
`username` 用户名、`email` 邮箱、`phone` 电话、`province` 省份、`city` 城市、
`address` 地址、`zip` 邮编、`company` 公司、`job` 职业、`product` 商品、
`price` 价格、`date` 日期、`datetime` 日期时间、`ip`、`url`、`active` 是否有效、`note` 备注。

## 开发

```bash
npm run build   # tsc 编译到 dist/
npm test        # 编译 + node --test
```

目录结构：

```
src/
  cli.tsx      # 命令行入口（参数解析 / 非交互生成 / 启动 TUI）
  app.tsx      # Ink TUI 向导（预设→字段→行数→保存位置→…→确认）
  fields.ts    # 字段注册表（每个字段对应一个 faker 生成器）
  presets.ts   # 学生表 / 用户表 / 员工表 / 订单表预设
  generate.ts  # 数据集生成 + 写文件
  csv.ts       # CSV 转义 / 拼接
  test/        # node:test 单元测试
```

加新字段只需在 `src/fields.ts` 的 `FIELDS` 里追加一项（key + 中文名 + faker 生成器），TUI 与 `--list-fields` 会自动出现。
