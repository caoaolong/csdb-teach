export type DelimiterName = 'comma' | 'semicolon' | 'tab';

export const DELIMITERS: Record<DelimiterName, string> = {
  comma: ',',
  semicolon: ';',
  tab: '\t',
};

export const DELIMITER_LABELS: Record<DelimiterName, string> = {
  comma: '逗号 (,)',
  semicolon: '分号 (;)',
  tab: '制表符 (Tab)',
};

export function parseDelimiter(input: string): string {
  const v = input.trim().toLowerCase();
  if (v === 'tab' || v === '\\t' || v === '\t') return '\t';
  if (v === 'semicolon' || v === ';' || v === '分号') return ';';
  if (v === 'comma' || v === ',' || v === '逗号' || v === '') return ',';
  // 允许直接传入单个自定义分隔符
  if (input.length === 1) return input;
  throw new Error(`不支持的分隔符: ${input}（可用 comma / semicolon / tab）`);
}

/** 对单个单元格做 CSV 转义（RFC 4180） */
export function escapeCell(value: string | number, delimiter: string): string {
  const s = String(value);
  if (s.includes('"') || s.includes('\n') || s.includes('\r') || s.includes(delimiter)) {
    return `"${s.replace(/"/g, '""')}"`;
  }
  return s;
}

export interface CsvOptions {
  delimiter: string;
  includeHeader: boolean;
}

export function toCsv(headers: string[], rows: Array<Array<string | number>>, opts: CsvOptions): string {
  const lines: string[] = [];
  if (opts.includeHeader) {
    lines.push(headers.map((h) => escapeCell(h, opts.delimiter)).join(opts.delimiter));
  }
  for (const row of rows) {
    lines.push(row.map((c) => escapeCell(c, opts.delimiter)).join(opts.delimiter));
  }
  return lines.join('\n') + '\n';
}
