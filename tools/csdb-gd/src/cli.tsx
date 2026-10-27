#!/usr/bin/env node
import { Command } from 'commander';
import React from 'react';
import { render } from 'ink';
import App, { DEFAULT_INITIAL, type InitialConfig } from './app.js';
import { FIELDS, parseFieldList } from './fields.js';
import { PRESETS, getPreset } from './presets.js';
import {
  defaultOutFileName,
  generateDataset,
  writeCsvFile,
  type EncodingName,
  type LocaleName,
} from './generate.js';
import { parseDelimiter } from './csv.js';

const program = new Command();

program
  .name('csdb-gd')
  .description('使用 faker.js 生成模拟数据并保存为 CSV（含 TUI 交互配置）')
  .version('0.1.0')
  .option('--preset <name>', `预设模板: ${PRESETS.map((p) => p.key).join(' / ')}`)
  .option('--fields <list>', '字段列表，逗号分隔（会覆盖 --preset）')
  .option('--rows <n>', '生成行数（1~1000000）', '100')
  .option('--out <path>', '保存位置（.csv 文件路径）')
  .option('--delimiter <d>', '分隔符: comma / semicolon / tab', 'comma')
  .option('--no-header', '不写入表头行（默认写入）')
  .option('--locale <l>', '数据语言: zh_CN / en', 'zh_CN')
  .option('--encoding <e>', '文件编码: utf8 / utf8-bom', 'utf8-bom')
  .option('--seed <s>', '随机种子（相同种子生成相同数据）')
  .option('-y, --yes', '非交互模式：跳过 TUI，直接按参数生成')
  .option('--list-fields', '列出所有可用字段后退出')
  .option('--list-presets', '列出所有预设模板后退出')
  .parse(process.argv);

const opts = program.opts<{
  preset?: string;
  fields?: string;
  rows: string;
  out?: string;
  delimiter: string;
  header: boolean;
  locale: string;
  encoding: string;
  seed?: string;
  yes?: boolean;
  listFields?: boolean;
  listPresets?: boolean;
}>();

if (opts.listFields) {
  for (const f of FIELDS) {
    console.log(`${f.key}\t${f.label}\t${f.desc}`);
  }
  process.exit(0);
}

if (opts.listPresets) {
  for (const p of PRESETS) {
    console.log(`${p.key}\t${p.label}\t${p.desc}`);
  }
  process.exit(0);
}

function fail(msg: string): never {
  console.error(`错误: ${msg}`);
  process.exit(1);
}

/** 解析 --fields / --preset，返回预选字段（均未提供时返回 null） */
function resolveFlagFields(): { preselected: InitialConfig['preselectedFields']; note: string } {
  if (opts.fields) {
    const { fields, invalid } = parseFieldList(opts.fields);
    if (invalid.length > 0) fail(`未知字段: ${invalid.join(', ')}（可用 --list-fields 查看）`);
    if (fields.length === 0) fail('--fields 至少需要指定 1 个字段');
    return { preselected: fields, note: '--fields' };
  }
  if (opts.preset) {
    const p = getPreset(opts.preset);
    if (!p) fail(`未知预设: ${opts.preset}（可用 --list-presets 查看）`);
    if (p!.key === 'custom') return { preselected: FIELDS.map((f) => f.key), note: '--preset custom（全字段）' };
    return { preselected: [...p!.fields], note: `--preset ${p!.key}` };
  }
  return { preselected: null, note: '' };
}

function parseRows(s: string): number {
  const n = Number(s);
  if (!Number.isInteger(n) || n < 1 || n > 1_000_000) fail(`行数必须是 1~1000000 的整数，当前: ${s}`);
  return n;
}

function parseLocale(s: string): LocaleName {
  if (s === 'zh_CN' || s === 'en') return s;
  fail(`语言必须是 zh_CN / en，当前: ${s}`);
}

function parseEncoding(s: string): EncodingName {
  if (s === 'utf8' || s === 'utf8-bom') return s;
  fail(`编码必须是 utf8 / utf8-bom，当前: ${s}`);
}

if (opts.yes) {
  // ---------------- 非交互模式 ----------------
  const { preselected, note } = resolveFlagFields();
  const fields = preselected ?? [...getPreset('student')!.fields];
  const rows = parseRows(opts.rows);
  let delimiter: string;
  try {
    delimiter = parseDelimiter(opts.delimiter);
  } catch (e) {
    fail(e instanceof Error ? e.message : String(e));
  }
  const locale = parseLocale(opts.locale);
  const encoding = parseEncoding(opts.encoding);
  const out = opts.out ?? defaultOutFileName();
  try {
    const { csv } = generateDataset({
      fields,
      rows,
      locale,
      seed: opts.seed,
      delimiter: delimiter!,
      includeHeader: opts.header,
    });
    const r = writeCsvFile(out, csv, encoding);
    console.log(`✔ 已生成 ${rows} 行 x ${fields.length} 列 -> ${r.filePath}`);
    console.log(`  字段: ${fields.join(', ')}`);
    console.log(`  来源: ${note === '' ? '默认 student 预设' : note} · 大小: ${(r.bytes / 1024).toFixed(1)} KB`);
  } catch (e) {
    fail(e instanceof Error ? e.message : String(e));
  }
} else {
  // ---------------- TUI 交互模式 ----------------
  if (!process.stdin.isTTY) {
    fail('当前 stdin 不是 TTY，无法显示 TUI；请使用 -y/--yes 非交互模式');
  }
  const { preselected } = resolveFlagFields();
  const initial: InitialConfig = {
    ...DEFAULT_INITIAL,
    preselectedFields: preselected,
    rows: opts.rows ?? DEFAULT_INITIAL.rows,
    out: opts.out ?? DEFAULT_INITIAL.out,
    delimiter: (() => {
      try {
        const d = parseDelimiter(opts.delimiter);
        return (d === '\t' ? 'tab' : d === ';' ? 'semicolon' : 'comma') as InitialConfig['delimiter'];
      } catch {
        return DEFAULT_INITIAL.delimiter;
      }
    })(),
    includeHeader: opts.header,
    locale: opts.locale === 'en' ? 'en' : 'zh_CN',
    encoding: opts.encoding === 'utf8' ? 'utf8' : 'utf8-bom',
    seed: opts.seed ?? '',
  };
  if (opts.rows !== '100') {
    const n = Number(opts.rows);
    if (!Number.isInteger(n) || n < 1 || n > 1_000_000) {
      console.error(`警告: --rows 非法 (${opts.rows})，已回退默认值 100`);
      initial.rows = '100';
    }
  }
  render(<App initial={initial} />);
}
