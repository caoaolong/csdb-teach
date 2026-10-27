import { mkdirSync, writeFileSync } from 'node:fs';
import { dirname, isAbsolute, resolve } from 'node:path';
import { faker, fakerEN, fakerZH_CN, type Faker } from '@faker-js/faker';
import { FIELD_MAP, type FieldKey, type LocaleName } from './fields.js';
import { toCsv } from './csv.js';

export type { LocaleName };

export function getFaker(locale: LocaleName): Faker {
  return locale === 'en' ? fakerEN : fakerZH_CN;
}

export function baseFaker(): Faker {
  return faker;
}

/** 把 seed 字符串转成 faker.seed 需要的数字（纯数字直接用，否则做简单哈希） */
export function seedFromString(seed: string): number {
  const s = seed.trim();
  if (/^-?\d+$/.test(s)) return Number(s);
  let h = 0;
  for (let i = 0; i < s.length; i++) {
    h = (Math.imul(h, 31) + s.charCodeAt(i)) | 0;
  }
  return h;
}

export interface GenerateOptions {
  fields: FieldKey[];
  rows: number;
  locale: LocaleName;
  /** 为空表示每次随机 */
  seed?: string;
  delimiter: string;
  includeHeader: boolean;
}

export interface GenerateResult {
  headers: string[];
  rows: Array<Array<string | number>>;
  csv: string;
}

export function generateDataset(opts: GenerateOptions): GenerateResult {
  if (opts.fields.length === 0) throw new Error('至少需要选择 1 个字段');
  if (!Number.isInteger(opts.rows) || opts.rows < 1 || opts.rows > 1_000_000) {
    throw new Error(`行数必须是 1~1000000 的整数，当前: ${opts.rows}`);
  }
  const f = getFaker(opts.locale);
  if (opts.seed !== undefined && opts.seed !== '') {
    f.seed(seedFromString(opts.seed));
  }
  const defs = opts.fields.map((k) => {
    const d = FIELD_MAP[k];
    if (!d) throw new Error(`未知字段: ${k}`);
    return d;
  });
  const headers = defs.map((d) => d.header);
  const rows: Array<Array<string | number>> = [];
  for (let i = 0; i < opts.rows; i++) {
    rows.push(defs.map((d) => d.generate(f, i, opts.locale)));
  }
  const csv = toCsv(headers, rows, { delimiter: opts.delimiter, includeHeader: opts.includeHeader });
  return { headers, rows, csv };
}

export type EncodingName = 'utf8' | 'utf8-bom';

export interface WriteResult {
  /** 解析后的绝对路径 */
  filePath: string;
  bytes: number;
}

/** 默认输出文件名：csdb-data-YYYYMMDD-HHmmss.csv */
export function defaultOutFileName(now = new Date()): string {
  const p = (n: number) => String(n).padStart(2, '0');
  const stamp = `${now.getFullYear()}${p(now.getMonth() + 1)}${p(now.getDate())}-${p(now.getHours())}${p(
    now.getMinutes(),
  )}${p(now.getSeconds())}`;
  return `csdb-data-${stamp}.csv`;
}

/** 写 CSV 文件（自动创建父目录；utf8-bom 会写入 BOM，方便 Excel 打开中文） */
export function writeCsvFile(out: string, csv: string, encoding: EncodingName): WriteResult {
  let filePath = out.trim();
  if (filePath === '') throw new Error('保存位置不能为空');
  if (!isAbsolute(filePath)) filePath = resolve(process.cwd(), filePath);
  if (!filePath.toLowerCase().endsWith('.csv')) {
    throw new Error(`保存位置必须以 .csv 结尾，当前: ${out}`);
  }
  mkdirSync(dirname(filePath), { recursive: true });
  const content = encoding === 'utf8-bom' ? '﻿' + csv : csv;
  writeFileSync(filePath, content, 'utf8');
  return { filePath, bytes: Buffer.byteLength(content, 'utf8') };
}
