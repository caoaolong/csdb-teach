#!/usr/bin/env node
import { Command } from 'commander';
import React from 'react';
import { render } from 'ink';
import App from './app.js';
import {
  DBD_HEADER_SIZE,
  DBD_ROW_SIZE,
  DBI_ENTRY_SIZE,
  DBI_HEADER_SIZE,
  DBD_FIELDS,
  detectKind,
  fmtOff,
  hexDump,
  loadFile,
  parseDbdHeader,
  parseDbdRow,
  parseDbiEntry,
  parseDbiHeader,
  rowToCsv,
} from './format.js';

const program = new Command();

program
  .name('csdb-fm')
  .description('可视化查看 csdb-teach 二进制格式（.dbd 数据文件 / .dbi 索引文件）')
  .version('0.1.0')
  .argument('<file>', '.dbd 或 .dbi 文件路径')
  .option('--head', '只打印文件头后退出')
  .option('--row <n>', '只打印第 N 行/条（0 起）后退出')
  .option('--hex <off,len>', '只打印 [off, off+len) 的 hex 后退出（off 支持 0x 前缀）')
  .option('--dump [limit]', '逐行转储全部记录后退出（可限行数），适合脚本')
  .parse(process.argv);

const opts = program.opts<{ head?: boolean; row?: string; hex?: string; dump?: string | boolean }>();
const [file] = program.args as [string | undefined];

function fail(msg: string): never {
  console.error(`错误: ${msg}`);
  process.exit(1);
}

if (!file) fail('请给出 .dbd 或 .dbi 文件路径');

let loaded;
try {
  loaded = loadFile(file!);
} catch (e) {
  fail(e instanceof Error ? e.message : String(e));
}
const kind = detectKind(loaded!.buf);
if (kind === 'unknown') fail(`${file} 魔数不匹配，既不是 .dbd 也不是 .dbi`);

function parseNum(s: string, what: string): number {
  const n = s.startsWith('0x') || s.startsWith('0X') ? parseInt(s, 16) : Number(s);
  if (!Number.isInteger(n) || n < 0) fail(`${what} 非法: ${s}`);
  return n;
}

if (opts.head) {
  if (kind === 'dbd') {
    const h = parseDbdHeader(loaded!.buf);
    console.log(`kind=dbd magic=CSDBD001 version=${h.version} row_size=${h.rowSize} row_count=${String(h.rowCount)} reserved=${String(h.reserved)} bytes=${loaded!.size}`);
  } else {
    const h = parseDbiHeader(loaded!.buf);
    console.log(`kind=dbi magic=CSDBIDX1 csv_size=${String(h.csvSize)} csv_mtime=${String(h.csvMtime)} count=${String(h.count)} bytes=${loaded!.size}`);
  }
  process.exit(0);
}

if (opts.row !== undefined) {
  const n = parseNum(opts.row, '--row');
  if (kind === 'dbd') {
    const r = parseDbdRow(loaded!.buf, n);
    console.log(`#${n} @${r.offset}`);
    for (const f of DBD_FIELDS) {
      const v = f.name === 'id' ? String(r.id) : f.name === 'age' ? String(r.age) : f.name === 'score' ? String(r.score) : String((r as unknown as Record<string, unknown>)[snakeToCamel(f.name)]);
      console.log(`  +${f.off} ${f.name}(${f.type}) = ${v}`);
    }
    console.log(rowToCsv(r));
  } else {
    const e = parseDbiEntry(loaded!.buf, n);
    console.log(`#${n} @${DBI_HEADER_SIZE + n * DBI_ENTRY_SIZE} id=${String(e.id)} offset=${fmtOff(e.offset)}`);
  }
  process.exit(0);
}

function snakeToCamel(s: string): string {
  return s.replace(/_([a-z])/g, (_, c: string) => c.toUpperCase());
}

if (opts.hex !== undefined) {
  const [o, l] = opts.hex.split(',');
  if (o === undefined || l === undefined) fail('--hex 格式应为 off,len（off 支持 0x 前缀）');
  const off = parseNum(o!, 'off');
  const len = parseNum(l!, 'len');
  if (off + len > loaded!.buf.length) fail(`越界：文件共 ${loaded!.buf.length} 字节`);
  for (const line of hexDump(loaded!.buf.subarray(off, off + len), off)) console.log(line);
  process.exit(0);
}

if (opts.dump !== undefined) {
  const limit = typeof opts.dump !== 'string' ? Infinity : parseNum(opts.dump, '--dump');
  if (kind === 'dbd') {
    const h = parseDbdHeader(loaded!.buf);
    const total = Number(h.rowCount);
    for (let i = 0; i < total && i < limit; i++) console.log(rowToCsv(parseDbdRow(loaded!.buf, i)));
  } else {
    const h = parseDbiHeader(loaded!.buf);
    const total = Number(h.count);
    for (let i = 0; i < total && i < limit; i++) {
      const e = parseDbiEntry(loaded!.buf, i);
      console.log(`${String(e.id)},${String(e.offset)}`);
    }
  }
  process.exit(0);
}

// ---------------- TUI 交互模式 ----------------
if (!process.stdin.isTTY) {
  fail('当前 stdin 不是 TTY，无法显示 TUI；请用 --head / --row / --hex / --dump');
}
render(<App file={{ path: file!, kind: kind as 'dbd' | 'dbi', buf: loaded!.buf }} />);
