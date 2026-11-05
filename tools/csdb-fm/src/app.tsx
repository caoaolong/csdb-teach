import React, { useState } from 'react';
import { Box, Text, useApp, useInput } from 'ink';
import {
  DBD_HEADER_SIZE,
  DBD_ROW_SIZE,
  DBI_ENTRY_SIZE,
  DBI_HEADER_SIZE,
  DBD_FIELDS,
  fmtOff,
  hexDump,
  parseDbdHeader,
  parseDbdRow,
  parseDbiEntry,
  parseDbiHeader,
} from './format.js';

export interface FmInput {
  path: string;
  kind: 'dbd' | 'dbi';
  buf: Buffer;
}

type View = 'header' | 'rows' | 'detail' | 'layout';

const VIEWS: Array<{ key: View; label: string }> = [
  { key: 'header', label: '文件头' },
  { key: 'rows', label: '记录' },
  { key: 'detail', label: '详情' },
  { key: 'layout', label: '布局' },
];

const WIN = 11; // 记录列表窗口行数

function clamp(n: number, lo: number, hi: number): number {
  return Math.min(hi, Math.max(lo, n));
}

function mtimeStr(m: bigint): string {
  const t = Number(m);
  if (!Number.isFinite(t) || t < 0 || t > 4102444800) return String(m);
  return `${String(m)} (${new Date(t * 1000).toLocaleString()})`;
}

function HeaderView({ file }: { file: FmInput }) {
  const { buf, kind } = file;
  const rows: Array<[string, string, string]> =
    kind === 'dbd'
      ? (() => {
          const h = parseDbdHeader(buf);
          return [
            ['0', '8', `magic = ${buf.subarray(0, 8).toString('ascii')}`],
            ['8', '4', `version = ${h.version}`],
            ['12', '4', `row_size = ${h.rowSize}`],
            ['16', '8', `row_count = ${String(h.rowCount)}`],
            ['24', '8', `reserved = ${String(h.reserved)}`],
          ];
        })()
      : (() => {
          const h = parseDbiHeader(buf);
          return [
            ['0', '8', `magic = ${buf.subarray(0, 8).toString('ascii')}`],
            ['8', '8', `csv_size = ${String(h.csvSize)}`],
            ['16', '8', `csv_mtime = ${mtimeStr(h.csvMtime)}`],
            ['24', '8', `count = ${String(h.count)}`],
          ];
        })();
  return (
    <Box flexDirection="column">
      <Text bold>偏移 大小 字段</Text>
      {rows.map(([off, size, v], i) => (
        <Text key={i}>
          <Text color="cyan">{off.padStart(6)}</Text> {size.padStart(4)}  {v}
        </Text>
      ))}
      <Box marginTop={1} flexDirection="column">
        <Text dimColor>-- 原始字节 --</Text>
        {hexDump(buf.subarray(0, kind === 'dbd' ? DBD_HEADER_SIZE : DBI_HEADER_SIZE), 0).map((l, i) => (
          <Text key={i}>{l}</Text>
        ))}
      </Box>
    </Box>
  );
}

function fieldVal(row: Record<string, string | number | bigint>, name: string): string {
  const v = row[name];
  return typeof v === 'bigint' ? v.toString() : String(v);
}

function RowsView({ file, cursor }: { file: FmInput; cursor: number }) {
  const { buf, kind } = file;
  const count =
    kind === 'dbd' ? Number(parseDbdHeader(buf).rowCount) : Number(parseDbiHeader(buf).count);
  if (count === 0) return <Text dimColor>空文件（0 行记录）</Text>;
  const start = clamp(cursor - Math.floor(WIN / 2), 0, Math.max(0, count - WIN));
  const lines: React.ReactNode[] = [];
  for (let i = start; i < Math.min(start + WIN, count); i++) {
    const on = i === cursor;
    const label =
      kind === 'dbd'
        ? (() => {
            const r = parseDbdRow(buf, i);
            return `#${i} @${DBD_HEADER_SIZE + i * DBD_ROW_SIZE}  id=${String(r.id)}  ${r.studentNo}  ${r.name}`;
          })()
        : (() => {
            const e = parseDbiEntry(buf, i);
            return `#${i} @${DBI_HEADER_SIZE + i * DBI_ENTRY_SIZE}  id=${String(e.id)}  offset=${String(e.offset)}`;
          })();
    lines.push(
      <Text key={i} color={on ? 'green' : undefined} bold={on}>
        {on ? '❯ ' : '  '}
        {label}
      </Text>,
    );
  }
  return (
    <Box flexDirection="column">
      <Text dimColor>
        共 {count} 条 · 光标 #{cursor}/{count - 1}
      </Text>
      {lines}
    </Box>
  );
}

function DetailView({ file, cursor }: { file: FmInput; cursor: number }) {
  const { buf, kind } = file;
  if (kind === 'dbi') {
    const e = parseDbiEntry(buf, cursor);
    const base = DBI_HEADER_SIZE + cursor * DBI_ENTRY_SIZE;
    return (
      <Box flexDirection="column">
        <Text bold>条目 #{cursor}（文件偏移 {fmtOff(base)}，{DBI_ENTRY_SIZE} 字节）</Text>
        <Text>
          <Text color="cyan">+0 宽 8</Text> id = {String(e.id)}
        </Text>
        <Text>
          <Text color="cyan">+8 宽 8</Text> offset = {fmtOff(e.offset)}（指向源 CSV 字节）
        </Text>
        <Box marginTop={1} flexDirection="column">
          {hexDump(buf.subarray(base, base + DBI_ENTRY_SIZE), base).map((l, i) => (
            <Text key={i}>{l}</Text>
          ))}
        </Box>
      </Box>
    );
  }
  const r = parseDbdRow(buf, cursor);
  const rec = r as unknown as Record<string, string | number | bigint>;
  return (
    <Box flexDirection="column">
      <Text bold>
        行 #{cursor}（文件偏移 {fmtOff(r.offset)}，{DBD_ROW_SIZE} 字节）
      </Text>
      {DBD_FIELDS.map((f) => (
        <Text key={f.name}>
          <Text color="cyan">
            +{String(f.off).padStart(3)} 宽{String(f.size).padStart(3)}
          </Text>{' '}
          {f.name} ({f.type}) = {fieldVal(rec, camel(f.name))}
        </Text>
      ))}
      <Box marginTop={1} flexDirection="column">
        <Text dimColor>-- 行原始字节 --</Text>
        {hexDump(buf.subarray(r.offset, r.offset + DBD_ROW_SIZE), r.offset).map((l, i) => (
          <Text key={i}>{l}</Text>
        ))}
      </Box>
    </Box>
  );
}

/** student_no -> studentNo，其余保持 */
function camel(name: string): string {
  return name.replace(/_([a-z])/g, (_, c: string) => c.toUpperCase());
}

function LayoutView({ file, cursor }: { file: FmInput; cursor: number }) {
  const { buf, kind, path } = file;
  if (kind === 'dbi') {
    const count = Number(parseDbiHeader(buf).count);
    const marks = [0, count - 1, cursor].filter((v, i, a) => v >= 0 && v < count && a.indexOf(v) === i);
    return (
      <Box flexDirection="column">
        <Text>[0 .. 31] 文件头 32B magic=CSDBIDX1（csv_size/csv_mtime/count）</Text>
        <Text>[32 .. {32 + count * DBI_ENTRY_SIZE - 1}] 条目区 {count} × {DBI_ENTRY_SIZE}B（已按 id 排序）</Text>
        {marks.map((i) => {
          const e = parseDbiEntry(buf, i);
          const off = DBI_HEADER_SIZE + i * DBI_ENTRY_SIZE;
          return (
            <Text key={i} color={i === cursor ? 'green' : undefined}>
              [{off} .. {off + 15}] 条目 #{i} id={String(e.id)} offset={String(e.offset)}
              {i === cursor ? '  <-- 你在这里' : ''}
            </Text>
          );
        })}
        <Text dimColor>公式：条目偏移 = 32 + i × 16 · 文件共 {buf.length} 字节（{path}）</Text>
      </Box>
    );
  }
  const h = parseDbdHeader(buf);
  const count = Number(h.rowCount);
  const marks = [0, count - 1, cursor].filter((v, i, a) => v >= 0 && v < count && a.indexOf(v) === i);
  return (
    <Box flexDirection="column">
      <Text>[0 .. 31] 文件头 32B magic=CSDBD001 version={h.version}</Text>
      <Text>
        [32 .. {32 + count * DBD_ROW_SIZE - 1}] 记录区 {count} × {DBD_ROW_SIZE}B
      </Text>
      {marks.map((i) => {
        const off = DBD_HEADER_SIZE + i * DBD_ROW_SIZE;
        return (
          <Text key={i} color={i === cursor ? 'green' : undefined}>
            [{off} .. {off + DBD_ROW_SIZE - 1}] 行 #{i}
            {i === cursor ? '  <-- 你在这里' : ''}
          </Text>
        );
      })}
      <Text dimColor>
        公式：行偏移 = 32 + rid × {DBD_ROW_SIZE} · 行定长所以改值不移位 · 文件共 {buf.length}{' '}
        字节（{path}）
      </Text>
    </Box>
  );
}

export default function App({ file }: { file: FmInput }) {
  const { exit } = useApp();
  const [view, setView] = useState<View>('header');
  const [cursor, setCursor] = useState(0);
  const count =
    file.kind === 'dbd'
      ? Number(parseDbdHeader(file.buf).rowCount)
      : Number(parseDbiHeader(file.buf).count);
  const max = Math.max(0, count - 1);

  useInput((input, key) => {
    if (input === '1') return setView('header');
    if (input === '2') return setView('rows');
    if (input === '3') return setView('detail');
    if (input === '4') return setView('layout');
    if (input === 'q' || key.escape) return exit();
    if (view !== 'rows' && view !== 'detail') {
      if (key.return && view === 'header') return setView('rows');
      return;
    }
    if (key.upArrow || input === 'k') setCursor((c) => clamp(c - 1, 0, max));
    else if (key.downArrow || input === 'j') setCursor((c) => clamp(c + 1, 0, max));
    else if (key.pageUp) setCursor((c) => clamp(c - WIN, 0, max));
    else if (key.pageDown) setCursor((c) => clamp(c + WIN, 0, max));
    else if (input === 'g') setCursor(0);
    else if (input === 'G') setCursor(max);
    else if (key.return && view === 'rows') setView('detail');
  });

  const vi = VIEWS.findIndex((v) => v.key === view);
  return (
    <Box flexDirection="column">
      <Box marginBottom={1}>
        <Text bold color="cyan">
          {file.kind === 'dbd' ? '◉ .dbd 数据文件' : '◉ .dbi 索引文件'}
        </Text>
        <Text dimColor> {file.path}</Text>
      </Box>
      <Box marginBottom={1}>
        {VIEWS.map((v, i) => (
          <Text key={v.key} color={i === vi ? 'green' : undefined} bold={i === vi}>
            {` ${i + 1}.${v.label} `}
          </Text>
        ))}
      </Box>
      {view === 'header' && <HeaderView file={file} />}
      {view === 'rows' && <RowsView file={file} cursor={cursor} />}
      {view === 'detail' && (count === 0 ? <Text dimColor>空文件（0 行记录）</Text> : <DetailView file={file} cursor={cursor} />)}
      {view === 'layout' && <LayoutView file={file} cursor={cursor} />}
      <Box marginTop={1}>
        <Text dimColor>
          {view === 'rows' || view === 'detail'
            ? '↑↓/jk 移动 · PgUp/PgDn 翻页 · g/G 首尾 · 回车详情 · 1-4 切换 · q 退出'
            : '1-4 切换视图 · 回车进记录 · q 退出'}
        </Text>
      </Box>
    </Box>
  );
}
