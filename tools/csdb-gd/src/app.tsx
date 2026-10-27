import React, { useState } from 'react';
import { Box, Text, useApp, useInput } from 'ink';
import { FIELDS, type FieldKey } from './fields.js';
import { PRESETS } from './presets.js';
import {
  defaultOutFileName,
  generateDataset,
  writeCsvFile,
  type EncodingName,
  type LocaleName,
} from './generate.js';
import { DELIMITER_LABELS, type DelimiterName } from './csv.js';

/* ---------------------------------- 类型 ---------------------------------- */

export interface InitialConfig {
  /** 命令行 --fields / --preset 传入时预选字段，并跳过“选预设”一步；null = 未预选 */
  preselectedFields: FieldKey[] | null;
  rows: string;
  out: string;
  delimiter: DelimiterName;
  includeHeader: boolean;
  locale: LocaleName;
  encoding: EncodingName;
  seed: string;
}

export const DEFAULT_INITIAL: InitialConfig = {
  preselectedFields: null,
  rows: '100',
  out: defaultOutFileName(),
  delimiter: 'comma',
  includeHeader: true,
  locale: 'zh_CN',
  encoding: 'utf8-bom',
  seed: '',
};

type Step =
  | 'preset'
  | 'fields'
  | 'rows'
  | 'out'
  | 'delimiter'
  | 'header'
  | 'locale'
  | 'encoding'
  | 'seed'
  | 'confirm'
  | 'done';

const STEP_ORDER: Step[] = [
  'preset',
  'fields',
  'rows',
  'out',
  'delimiter',
  'header',
  'locale',
  'encoding',
  'seed',
  'confirm',
];

const STEP_TITLES: Record<Step, string> = {
  preset: '选择预设模板',
  fields: '选择字段（空格 勾选/取消，a 全选，n 全不选）',
  rows: '生成行数',
  out: '保存位置（.csv 文件路径，不存在目录会自动创建）',
  delimiter: 'CSV 分隔符',
  header: '是否写入表头行',
  locale: '数据语言',
  encoding: '文件编码',
  seed: '随机种子（可空，回车跳过则每次随机）',
  confirm: '确认生成',
  done: '完成',
};

function prevStep(step: Step): Step | null {
  const i = STEP_ORDER.indexOf(step);
  if (i <= 0) return null;
  return STEP_ORDER[i - 1]!;
}

/* -------------------------------- 通用组件 -------------------------------- */

function StepHeader({ step, index, total }: { step: Step; index: number; total: number }) {
  return (
    <Box flexDirection="column" marginBottom={1}>
      <Text color="cyan" bold>
        [{index + 1}/{total}] {STEP_TITLES[step]}
      </Text>
      <Text dimColor>↑↓ 移动 · 回车 确认 · Esc 返回上一步</Text>
    </Box>
  );
}

interface MenuItem<T> {
  label: string;
  hint?: string;
  value: T;
}

function Menu<T>({
  items,
  initialIndex = 0,
  onSubmit,
  onBack,
}: {
  items: Array<MenuItem<T>>;
  initialIndex?: number;
  onSubmit: (value: T) => void;
  onBack?: () => void;
}) {
  const [cursor, setCursor] = useState(initialIndex);
  useInput((input, key) => {
    if (key.upArrow) setCursor((c) => (c - 1 + items.length) % items.length);
    else if (key.downArrow) setCursor((c) => (c + 1) % items.length);
    else if (key.return) onSubmit(items[cursor]!.value);
    else if (key.escape && onBack) onBack();
  });
  return (
    <Box flexDirection="column">
      {items.map((it, i) => (
        <Box key={i}>
          <Text color={i === cursor ? 'green' : undefined} bold={i === cursor}>
            {i === cursor ? '❯ ' : '  '}
            {it.label}
          </Text>
          {it.hint ? <Text dimColor>  {it.hint}</Text> : null}
        </Box>
      ))}
    </Box>
  );
}

function FieldPicker({
  initial,
  onSubmit,
  onBack,
}: {
  initial: FieldKey[];
  onSubmit: (fields: FieldKey[]) => void;
  onBack: () => void;
}) {
  const [cursor, setCursor] = useState(0);
  const [checked, setChecked] = useState<FieldKey[]>(initial);
  const [warn, setWarn] = useState('');
  useInput((input, key) => {
    if (key.upArrow) setCursor((c) => (c - 1 + FIELDS.length) % FIELDS.length);
    else if (key.downArrow) setCursor((c) => (c + 1) % FIELDS.length);
    else if (key.escape) onBack();
    else if (key.return) {
      if (checked.length === 0) {
        setWarn('至少勾选 1 个字段');
        return;
      }
      // 保持 FIELDS 定义顺序
      onSubmit(FIELDS.map((f) => f.key).filter((k) => checked.includes(k)));
    } else if (input === ' ') {
      const k = FIELDS[cursor]!.key;
      setWarn('');
      setChecked((prev) => (prev.includes(k) ? prev.filter((x) => x !== k) : [...prev, k]));
    } else if (input === 'a') {
      setWarn('');
      setChecked(FIELDS.map((f) => f.key));
    } else if (input === 'n') {
      setChecked([]);
    }
  });
  return (
    <Box flexDirection="column">
      <Text dimColor>
        已选 {checked.length}/{FIELDS.length} 个
      </Text>
      {FIELDS.map((f, i) => {
        const on = checked.includes(f.key);
        return (
          <Box key={f.key}>
            <Text color={i === cursor ? 'green' : undefined} bold={i === cursor}>
              {i === cursor ? '❯ ' : '  '}[{on ? 'x' : ' '}] {f.label}
            </Text>
            <Text dimColor>  {f.desc}</Text>
          </Box>
        );
      })}
      {warn ? <Text color="red">{warn}</Text> : null}
    </Box>
  );
}

function Prompt({
  initial,
  placeholder,
  validate,
  onSubmit,
  onBack,
}: {
  initial: string;
  placeholder?: string;
  validate: (v: string) => string | null;
  onSubmit: (v: string) => void;
  onBack: () => void;
}) {
  const [value, setValue] = useState(initial);
  const [err, setErr] = useState('');
  useInput((input, key) => {
    if (key.return) {
      const msg = validate(value);
      if (msg) {
        setErr(msg);
        return;
      }
      setErr('');
      onSubmit(value);
    } else if (key.escape) {
      onBack();
    } else if (key.backspace || key.delete) {
      setErr('');
      setValue((v) => v.slice(0, -1));
    } else if (!key.ctrl && !key.meta && input) {
      setErr('');
      // 过滤控制字符，允许粘贴多字符
      const clean = input.replace(/[\u0000-\u001F\u007F]/g, '');
      if (clean) setValue((v) => v + clean);
    }
  });
  return (
    <Box flexDirection="column">
      <Box>
        <Text color="green" bold>
          ❯{' '}
        </Text>
        <Text>
          {value}
          <Text color="green">▊</Text>
        </Text>
        {value === '' && placeholder ? <Text dimColor> {placeholder}</Text> : null}
      </Box>
      {err ? <Text color="red">{err}</Text> : null}
    </Box>
  );
}

/* --------------------------------- 主 App --------------------------------- */

const DELIMITER_OPTIONS: Array<MenuItem<DelimiterName>> = (
  Object.keys(DELIMITER_LABELS) as DelimiterName[]
).map((k) => ({ label: DELIMITER_LABELS[k]!, value: k }));

export default function App({ initial }: { initial: InitialConfig }) {
  const { exit } = useApp();
  const [step, setStep] = useState<Step>(initial.preselectedFields ? 'fields' : 'preset');
  const [fields, setFields] = useState<FieldKey[]>(
    initial.preselectedFields ?? PRESETS[0]!.fields,
  );
  const [rows, setRows] = useState(initial.rows);
  const [out, setOut] = useState(initial.out);
  const [delimiter, setDelimiter] = useState<DelimiterName>(initial.delimiter);
  const [includeHeader, setIncludeHeader] = useState(initial.includeHeader);
  const [locale, setLocale] = useState<LocaleName>(initial.locale);
  const [encoding, setEncoding] = useState<EncodingName>(initial.encoding);
  const [seed, setSeed] = useState(initial.seed);
  const [doneInfo, setDoneInfo] = useState<{ filePath: string; bytes: number } | null>(null);
  const [doneError, setDoneError] = useState('');

  const go = (s: Step) => setStep(s);
  const back = () => {
    const p = prevStep(step);
    // 若跳过了 preset（命令行已指定字段），从 fields 返回时直接退出选择
    if (step === 'fields' && initial.preselectedFields) {
      exit();
      return;
    }
    if (p) {
      // 同样跳过 preset
      if (p === 'preset' && initial.preselectedFields) {
        exit();
        return;
      }
      setStep(p);
    }
  };

  const stepIndex = step === 'done' ? STEP_ORDER.length : STEP_ORDER.indexOf(step);

  const doGenerate = () => {
    try {
      const n = Number(rows);
      const { csv } = generateDataset({
        fields,
        rows: n,
        locale,
        seed: seed.trim() === '' ? undefined : seed,
        delimiter:
          delimiter === 'tab' ? '\t' : delimiter === 'semicolon' ? ';' : ',',
        includeHeader,
      });
      const r = writeCsvFile(out, csv, encoding);
      setDoneInfo({ filePath: r.filePath, bytes: r.bytes });
      setDoneError('');
    } catch (e) {
      setDoneInfo(null);
      setDoneError(e instanceof Error ? e.message : String(e));
    }
    go('done');
  };

  if (step === 'done') {
    return (
      <Box flexDirection="column">
        {doneInfo ? (
          <Box flexDirection="column">
            <Text color="green" bold>
              ✔ 生成成功！
            </Text>
            <Text>文件：{doneInfo.filePath}</Text>
            <Text>
              行数：{rows} · 字段：{fields.length} 个 · 大小：
              {(doneInfo.bytes / 1024).toFixed(1)} KB
            </Text>
            <Text dimColor>回车退出</Text>
            <DoneKey onDone={exit} />
          </Box>
        ) : (
          <Box flexDirection="column">
            <Text color="red" bold>
              ✘ 生成失败：{doneError}
            </Text>
            <Text dimColor>回车返回修改配置，Esc 退出</Text>
            <FailKey
              onRetry={() => go('confirm')}
              onExit={exit}
            />
          </Box>
        )}
      </Box>
    );
  }

  return (
    <Box flexDirection="column">
      <StepHeader step={step} index={stepIndex} total={STEP_ORDER.length} />

      {step === 'preset' && (
        <Menu
          items={PRESETS.map((p) => ({
            label: p.label,
            hint: `${p.desc}${p.fields.length > 0 ? `（${p.fields.length} 个字段）` : ''}`,
            value: p.key,
          }))}
          onSubmit={(key) => {
            const p = PRESETS.find((x) => x.key === key)!;
            setFields(p.fields.length > 0 ? p.fields : FIELDS.map((f) => f.key));
            go('fields');
          }}
        />
      )}

      {step === 'fields' && (
        <FieldPicker initial={fields} onSubmit={(f) => { setFields(f); go('rows'); }} onBack={back} />
      )}

      {step === 'rows' && (
        <Prompt
          initial={rows}
          placeholder="如 100"
          validate={(v) => {
            const n = Number(v.trim());
            if (!Number.isInteger(n) || n < 1 || n > 1_000_000) return '请输入 1~1000000 的整数';
            return null;
          }}
          onSubmit={(v) => { setRows(v.trim()); go('out'); }}
          onBack={back}
        />
      )}

      {step === 'out' && (
        <Prompt
          initial={out}
          placeholder="./data.csv"
          validate={(v) => {
            if (v.trim() === '') return '保存位置不能为空';
            if (!v.trim().toLowerCase().endsWith('.csv')) return '保存位置必须以 .csv 结尾';
            return null;
          }}
          onSubmit={(v) => { setOut(v.trim()); go('delimiter'); }}
          onBack={back}
        />
      )}

      {step === 'delimiter' && (
        <Menu
          items={DELIMITER_OPTIONS}
          initialIndex={DELIMITER_OPTIONS.findIndex((o) => o.value === delimiter)}
          onSubmit={(v) => { setDelimiter(v); go('header'); }}
          onBack={back}
        />
      )}

      {step === 'header' && (
        <Menu
          items={[
            { label: '是（推荐）', hint: '首行为字段名', value: 'yes' },
            { label: '否', hint: '直接从数据行开始', value: 'no' },
          ]}
          initialIndex={includeHeader ? 0 : 1}
          onSubmit={(v) => { setIncludeHeader(v === 'yes'); go('locale'); }}
          onBack={back}
        />
      )}

      {step === 'locale' && (
        <Menu
          items={[
            { label: '中文（zh_CN）', hint: '中文姓名 / 电话 / 地址', value: 'zh_CN' as LocaleName },
            { label: '英文（en）', hint: '英文姓名 / 地址', value: 'en' as LocaleName },
          ]}
          initialIndex={locale === 'en' ? 1 : 0}
          onSubmit={(v) => { setLocale(v); go('encoding'); }}
          onBack={back}
        />
      )}

      {step === 'encoding' && (
        <Menu
          items={[
            { label: 'UTF-8 with BOM', hint: '推荐：Excel 直接打开中文不乱码', value: 'utf8-bom' as EncodingName },
            { label: 'UTF-8', hint: '通用编码', value: 'utf8' as EncodingName },
          ]}
          initialIndex={encoding === 'utf8' ? 1 : 0}
          onSubmit={(v) => { setEncoding(v); go('seed'); }}
          onBack={back}
        />
      )}

      {step === 'seed' && (
        <Prompt
          initial={seed}
          placeholder="可空；相同种子生成相同数据"
          validate={() => null}
          onSubmit={(v) => { setSeed(v.trim()); go('confirm'); }}
          onBack={back}
        />
      )}

      {step === 'confirm' && (
        <Box flexDirection="column">
          <Box flexDirection="column" marginBottom={1} borderStyle="round" paddingX={1}>
            <Text>字段（{fields.length}）：{fields.join(', ')}</Text>
            <Text>行数：{rows}</Text>
            <Text>保存位置：{out}</Text>
            <Text>
              分隔符：{DELIMITER_LABELS[delimiter]} · 表头：{includeHeader ? '是' : '否'} · 语言：
              {locale} · 编码：{encoding} · 种子：{seed === '' ? '（随机）' : seed}
            </Text>
          </Box>
          <Menu
            items={[
              { label: '开始生成', value: 'go' },
              { label: '返回修改', value: 'back' },
            ]}
            onSubmit={(v) => {
              if (v === 'go') doGenerate();
              else back();
            }}
            onBack={back}
          />
        </Box>
      )}
    </Box>
  );
}

function DoneKey({ onDone }: { onDone: () => void }) {
  useInput((_input, key) => {
    if (key.return || key.escape) onDone();
  });
  return null;
}

function FailKey({ onRetry, onExit }: { onRetry: () => void; onExit: () => void }) {
  useInput((_input, key) => {
    if (key.return) onRetry();
    else if (key.escape) onExit();
  });
  return null;
}
