import assert from 'node:assert/strict';
import { describe, it } from 'node:test';
import {
  DBD_HEADER_SIZE,
  DBD_ROW_SIZE,
  DBI_ENTRY_SIZE,
  DBI_HEADER_SIZE,
  cstr,
  detectKind,
  escapeCell,
  hexDump,
  parseDbdHeader,
  parseDbdRow,
  parseDbiEntry,
  parseDbiHeader,
  rowToCsv,
} from '../format.js';

const enc = new TextEncoder();

/** 构造 .dbd 测试块：头 + 2 行（第 2 行含中文与零填充） */
function makeDbd(): Buffer {
  const buf = Buffer.alloc(DBD_HEADER_SIZE + 2 * DBD_ROW_SIZE);
  const dv = new DataView(buf.buffer, buf.byteOffset, buf.byteLength);
  buf.write('CSDBD001', 0, 'ascii');
  dv.setUint32(8, 1, true);
  dv.setUint32(12, DBD_ROW_SIZE, true);
  dv.setBigUint64(16, 2n, true);
  // 行 #0
  const b0 = DBD_HEADER_SIZE;
  dv.setBigInt64(b0, 7n, true);
  buf.write('20240001', b0 + 8, 'ascii');
  buf.write('alice', b0 + 24, 'ascii');
  buf.write('女', b0 + 88, 'utf8');
  dv.setInt32(b0 + 96, 20, true);
  buf.write('CS', b0 + 100, 'ascii');
  dv.setInt32(b0 + 180, 90, true);
  // 行 #1（含逗号的名字在二进制里原样存，不加引号）
  const b1 = DBD_HEADER_SIZE + DBD_ROW_SIZE;
  dv.setBigInt64(b1, 9n, true);
  buf.write('S002', b1 + 8, 'ascii');
  buf.write('doe, john', b1 + 24, 'ascii');
  dv.setInt32(b1 + 96, 21, true);
  dv.setInt32(b1 + 180, 80, true);
  return buf;
}

function makeDbi(): Buffer {
  const buf = Buffer.alloc(DBI_HEADER_SIZE + 2 * DBI_ENTRY_SIZE);
  const dv = new DataView(buf.buffer, buf.byteOffset, buf.byteLength);
  buf.write('CSDBIDX1', 0, 'ascii');
  dv.setBigInt64(8, 100n, true);
  dv.setBigInt64(16, 1700000000n, true);
  dv.setBigUint64(24, 2n, true);
  dv.setBigInt64(32, 7n, true);
  dv.setBigInt64(40, 19n, true);
  dv.setBigInt64(48, 9n, true);
  dv.setBigInt64(56, 44n, true);
  return buf;
}

describe('detectKind', () => {
  it('按魔数区分 dbd / dbi / unknown', () => {
    assert.equal(detectKind(makeDbd()), 'dbd');
    assert.equal(detectKind(makeDbi()), 'dbi');
    assert.equal(detectKind(Buffer.alloc(32)), 'unknown');
    assert.equal(detectKind(Buffer.alloc(3)), 'unknown');
  });
});

describe('dbd', () => {
  it('文件头解析正确', () => {
    const h = parseDbdHeader(makeDbd());
    assert.equal(h.version, 1);
    assert.equal(h.rowSize, 280);
    assert.equal(h.rowCount, 2n);
  });

  it('行解析正确（含中文与零填充）', () => {
    const r0 = parseDbdRow(makeDbd(), 0);
    assert.equal(r0.id, 7n);
    assert.equal(r0.studentNo, '20240001');
    assert.equal(r0.name, 'alice');
    assert.equal(r0.gender, '女');
    assert.equal(r0.age, 20);
    assert.equal(r0.score, 90);
    assert.equal(r0.offset, 32);
    const buf = makeDbd();
    const r1 = parseDbdRow(buf, 1);
    assert.equal(r1.name, 'doe, john');
    assert.equal(r1.offset, 312);
  });

  it('越界行抛 RangeError', () => {
    assert.throws(() => parseDbdRow(makeDbd(), 2), RangeError);
  });

  it('cstr 读到 NUL 截断', () => {
    const b = Buffer.from([...enc.encode('ab'), 0, 0, ...enc.encode('zz')]);
    assert.equal(cstr(b, 0, 6), 'ab');
  });

  it('rowToCsv 与 C 端输出格式一致（含转义）', () => {
    const csv = rowToCsv(parseDbdRow(makeDbd(), 1));
    assert.ok(csv.startsWith('9,S002,"doe, john",'));
  });

  it('escapeCell 规则与 C 端 csv_escape 一致', () => {
    assert.equal(escapeCell('plain'), 'plain');
    assert.equal(escapeCell('a,b'), '"a,b"');
    assert.equal(escapeCell('say "hi"'), '"say ""hi"""');
  });
});

describe('dbi', () => {
  it('文件头与条目解析正确', () => {
    const buf = makeDbi();
    const h = parseDbiHeader(buf);
    assert.equal(h.csvSize, 100n);
    assert.equal(h.csvMtime, 1700000000n);
    assert.equal(h.count, 2n);
    assert.deepEqual(parseDbiEntry(buf, 0), { id: 7n, offset: 19n });
    assert.deepEqual(parseDbiEntry(buf, 1), { id: 9n, offset: 44n });
  });

  it('越界条目抛 RangeError', () => {
    assert.throws(() => parseDbiEntry(makeDbi(), 2), RangeError);
  });
});

describe('hexDump', () => {
  it('16 字节一行，地址从 base 起', () => {
    const lines = hexDump(Buffer.from([0x43, 0x53, 0x00, 0xff]), 0x20);
    assert.equal(lines.length, 1);
    assert.ok(lines[0]!.startsWith('00000020'));
    assert.ok(lines[0]!.includes('43 53 00 ff'));
    assert.ok(lines[0]!.endsWith('CS..'));
  });
});
