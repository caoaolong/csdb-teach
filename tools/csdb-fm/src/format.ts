import { readFileSync, statSync } from 'node:fs';

/**
 * csdb-teach 二进制格式解析（与 inc/record.h、inc/index.h 的 C 布局对应）。
 *
 * 前提：C 端按本机字节序 fwrite 结构体，x86-64 上即小端（LE），
 * 这里一律按 LE 解码。换大端机器需同步两边（教学暂不处理）。
 */

export const DBD_MAGIC = 'CSDBD001';
export const DBI_MAGIC = 'CSDBIDX1';
export const DBD_HEADER_SIZE = 32;
export const DBD_ROW_SIZE = 280;
export const DBI_HEADER_SIZE = 32;
export const DBI_ENTRY_SIZE = 16;

export type FileKind = 'dbd' | 'dbi' | 'unknown';

export function detectKind(buf: Buffer): FileKind {
  if (buf.length < 8) return 'unknown';
  const magic = buf.subarray(0, 8).toString('ascii');
  if (magic === DBD_MAGIC) return 'dbd';
  if (magic === DBI_MAGIC) return 'dbi';
  return 'unknown';
}

export interface LoadedFile {
  path: string;
  buf: Buffer;
  size: number;
}

export function loadFile(path: string): LoadedFile {
  const st = statSync(path);
  if (!st.isFile()) throw new Error(`不是普通文件: ${path}`);
  return { path, buf: readFileSync(path), size: st.size };
}

/* ------------------------------- .dbd ------------------------------- */

export interface DbdHeader {
  version: number;
  rowSize: number;
  rowCount: bigint;
  reserved: bigint;
}

export function parseDbdHeader(buf: Buffer): DbdHeader {
  if (buf.length < DBD_HEADER_SIZE) throw new Error(`文件过短（${buf.length} 字节），不是 .dbd`);
  if (detectKind(buf) !== 'dbd') throw new Error('魔数不匹配，不是 .dbd 数据文件');
  const dv = new DataView(buf.buffer, buf.byteOffset, buf.byteLength);
  return {
    version: dv.getUint32(8, true),
    rowSize: dv.getUint32(12, true),
    rowCount: dv.getBigUint64(16, true),
    reserved: dv.getBigUint64(24, true),
  };
}

/** 行内字段表（偏移/大小与 inc/record.h:student_t 一致） */
export const DBD_FIELDS = [
  { name: 'id', off: 0, size: 8, type: 'int64' },
  { name: 'student_no', off: 8, size: 16, type: 'char' },
  { name: 'name', off: 24, size: 64, type: 'char' },
  { name: 'gender', off: 88, size: 8, type: 'char' },
  { name: 'age', off: 96, size: 4, type: 'int32' },
  { name: 'major', off: 100, size: 32, type: 'char' },
  { name: 'grade', off: 132, size: 16, type: 'char' },
  { name: 'course', off: 148, size: 32, type: 'char' },
  { name: 'score', off: 180, size: 4, type: 'int32' },
  { name: 'email', off: 184, size: 64, type: 'char' },
  { name: 'phone', off: 248, size: 32, type: 'char' },
] as const;

export interface StudentRow {
  id: bigint;
  studentNo: string;
  name: string;
  gender: string;
  age: number;
  major: string;
  grade: string;
  course: string;
  score: number;
  email: string;
  phone: string;
  /** 该行首字节在文件中的绝对偏移 */
  offset: number;
}

/** 零填充 C 字符串：读到 NUL 为止，剩余忽略 */
export function cstr(buf: Buffer, off: number, len: number): string {
  const slice = buf.subarray(off, off + len);
  const nul = slice.indexOf(0);
  return slice.subarray(0, nul < 0 ? len : nul).toString('utf8');
}

export function parseDbdRow(buf: Buffer, index: number, rowSize: number = DBD_ROW_SIZE): StudentRow {
  if (index < 0) throw new RangeError(`行号越界: ${index}`);
  const base = DBD_HEADER_SIZE + index * rowSize;
  if (base + rowSize > buf.length) throw new RangeError(`行 #${index} 越界（文件 ${buf.length} 字节）`);
  const dv = new DataView(buf.buffer, buf.byteOffset + base, rowSize);
  return {
    id: dv.getBigInt64(0, true),
    studentNo: cstr(buf, base + 8, 16),
    name: cstr(buf, base + 24, 64),
    gender: cstr(buf, base + 88, 8),
    age: dv.getInt32(96, true),
    major: cstr(buf, base + 100, 32),
    grade: cstr(buf, base + 132, 16),
    course: cstr(buf, base + 148, 32),
    score: dv.getInt32(180, true),
    email: cstr(buf, base + 184, 64),
    phone: cstr(buf, base + 248, 32),
    offset: base,
  };
}

/** RFC4180 转义（与 C 端 csv_escape 一致），供 --dump 输出 CSV 行 */
export function escapeCell(v: string): string {
  if (v.includes('"') || v.includes(',') || v.includes('\n') || v.includes('\r')) {
    return `"${v.replace(/"/g, '""')}"`;
  }
  return v;
}

export function rowToCsv(r: StudentRow): string {
  return [
    String(r.id),
    escapeCell(r.studentNo),
    escapeCell(r.name),
    escapeCell(r.gender),
    String(r.age),
    escapeCell(r.major),
    escapeCell(r.grade),
    escapeCell(r.course),
    String(r.score),
    escapeCell(r.email),
    escapeCell(r.phone),
  ].join(',');
}

/* ------------------------------- .dbi ------------------------------- */

export interface DbiHeader {
  csvSize: bigint;
  csvMtime: bigint;
  count: bigint;
}

export function parseDbiHeader(buf: Buffer): DbiHeader {
  if (buf.length < DBI_HEADER_SIZE) throw new Error(`文件过短（${buf.length} 字节），不是 .dbi`);
  if (detectKind(buf) !== 'dbi') throw new Error('魔数不匹配，不是 .dbi 索引文件');
  const dv = new DataView(buf.buffer, buf.byteOffset, buf.byteLength);
  return {
    csvSize: dv.getBigInt64(8, true),
    csvMtime: dv.getBigInt64(16, true),
    count: dv.getBigUint64(24, true),
  };
}

export interface DbiEntry {
  id: bigint;
  offset: bigint;
}

export function parseDbiEntry(buf: Buffer, index: number): DbiEntry {
  if (index < 0) throw new RangeError(`条目号越界: ${index}`);
  const base = DBI_HEADER_SIZE + index * DBI_ENTRY_SIZE;
  if (base + DBI_ENTRY_SIZE > buf.length) throw new RangeError(`条目 #${index} 越界（文件 ${buf.length} 字节）`);
  const dv = new DataView(buf.buffer, buf.byteOffset + base, DBI_ENTRY_SIZE);
  return { id: dv.getBigInt64(0, true), offset: dv.getBigInt64(8, true) };
}

/* -------------------------------- hex ------------------------------- */

/** 经典 hex+ascii，每行 16 字节，base 为该段在文件中的起始偏移 */
export function hexDump(buf: Buffer, base: number = 0): string[] {
  const lines: string[] = [];
  for (let i = 0; i < buf.length; i += 16) {
    const chunk = buf.subarray(i, Math.min(i + 16, buf.length));
    const hex = [...chunk].map((b) => b.toString(16).padStart(2, '0')).join(' ');
    const ascii = [...chunk].map((b) => (b >= 32 && b < 127 ? String.fromCharCode(b) : '.')).join('');
    lines.push(`${(base + i).toString(16).padStart(8, '0')}  ${hex.padEnd(48)}  ${ascii}`);
  }
  return lines;
}

export function fmtOff(n: number | bigint): string {
  const s = typeof n === 'bigint' ? n.toString() : String(n);
  return `${s} (0x${BigInt(s).toString(16)})`;
}
