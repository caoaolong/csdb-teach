import assert from 'node:assert/strict';
import { describe, it } from 'node:test';
import { FIELDS, FIELD_MAP, isFieldKey, parseFieldList } from '../fields.js';
import { escapeCell, parseDelimiter, toCsv } from '../csv.js';
import { generateDataset, getFaker, seedFromString } from '../generate.js';
import { PRESETS } from '../presets.js';

describe('fields', () => {
  it('字段注册表非空且 key 唯一', () => {
    assert.ok(FIELDS.length > 0);
    const keys = FIELDS.map((f) => f.key);
    assert.equal(new Set(keys).size, keys.length);
  });

  it('parseFieldList 能识别合法/非法字段并去重', () => {
    const { fields, invalid } = parseFieldList('id,name, name,nope,id');
    assert.deepEqual(fields, ['id', 'name']);
    assert.deepEqual(invalid, ['nope']);
    assert.ok(isFieldKey('email'));
    assert.ok(!isFieldKey('nope'));
  });

  it('所有预设引用的字段都存在', () => {
    for (const p of PRESETS) {
      for (const k of p.fields) {
        assert.ok(k in FIELD_MAP, `preset ${p.key} 引用了未知字段 ${k}`);
      }
    }
  });
});

describe('csv', () => {
  it('含分隔符/引号/换行的单元格会被转义', () => {
    assert.equal(escapeCell('a,b', ','), '"a,b"');
    assert.equal(escapeCell('say "hi"', ','), '"say ""hi"""');
    assert.equal(escapeCell('l1\nl2', ','), '"l1\nl2"');
    assert.equal(escapeCell('plain', ','), 'plain');
  });

  it('parseDelimiter 支持三种命名分隔符', () => {
    assert.equal(parseDelimiter('comma'), ',');
    assert.equal(parseDelimiter('semicolon'), ';');
    assert.equal(parseDelimiter('tab'), '\t');
    assert.throws(() => parseDelimiter('pipe'));
  });

  it('toCsv 可选表头', () => {
    const withHeader = toCsv(['a', 'b'], [[1, 2]], { delimiter: ',', includeHeader: true });
    assert.equal(withHeader, 'a,b\n1,2\n');
    const noHeader = toCsv(['a', 'b'], [[1, 2]], { delimiter: ',', includeHeader: false });
    assert.equal(noHeader, '1,2\n');
  });
});

describe('generate', () => {
  it('按字段顺序生成指定行数', () => {
    const { headers, rows, csv } = generateDataset({
      fields: ['id', 'name', 'score'],
      rows: 5,
      locale: 'zh_CN',
      delimiter: ',',
      includeHeader: true,
    });
    assert.deepEqual(headers, ['id', 'name', 'score']);
    assert.equal(rows.length, 5);
    assert.equal(rows[0]![0], 1);
    assert.equal(rows[4]![0], 5);
    assert.ok(csv.startsWith(headers.join(',') + '\n'));
  });

  it('相同种子生成相同数据', () => {
    const opts = {
      fields: ['name', 'email', 'score'] as const,
      rows: 10,
      locale: 'zh_CN' as const,
      seed: 'csdb-1',
      delimiter: ',',
      includeHeader: true,
    };
    const a = generateDataset({ ...opts, fields: [...opts.fields] });
    const b = generateDataset({ ...opts, fields: [...opts.fields] });
    assert.equal(a.csv, b.csv);
  });

  it('空字段 / 非法行数抛错', () => {
    assert.throws(() => generateDataset({ fields: [], rows: 1, locale: 'zh_CN', delimiter: ',', includeHeader: true }));
    assert.throws(() =>
      generateDataset({ fields: ['id'], rows: 0, locale: 'zh_CN', delimiter: ',', includeHeader: true }),
    );
  });

  it('英文模式的是非值为 Yes / No', () => {
    const { rows } = generateDataset({
      fields: ['active'],
      rows: 20,
      locale: 'en',
      delimiter: ',',
      includeHeader: false,
    });
    for (const r of rows) {
      assert.ok(r[0] === 'Yes' || r[0] === 'No');
    }
  });

  it('en 语言实例可用', () => {
    const f = getFaker('en');
    assert.ok(typeof f.person.fullName() === 'string');
    assert.ok(Number.isInteger(seedFromString('123')));
    assert.ok(Number.isInteger(seedFromString('abc')));
  });
});
