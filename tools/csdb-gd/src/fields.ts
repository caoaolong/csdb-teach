import type { Faker } from '@faker-js/faker';

/** 数据语言：中文 / 英文 */
export type LocaleName = 'zh_CN' | 'en';

/** 所有可用字段的 key（同时用作 CSV 默认表头） */
export type FieldKey =
  | 'id'
  | 'uuid'
  | 'studentNo'
  | 'name'
  | 'gender'
  | 'age'
  | 'birthdate'
  | 'major'
  | 'grade'
  | 'course'
  | 'score'
  | 'username'
  | 'email'
  | 'phone'
  | 'province'
  | 'city'
  | 'address'
  | 'zip'
  | 'company'
  | 'job'
  | 'product'
  | 'price'
  | 'date'
  | 'datetime'
  | 'ip'
  | 'url'
  | 'active'
  | 'note';

export interface FieldDef {
  key: FieldKey;
  /** TUI 中显示的名称 */
  label: string;
  /** 一句话说明 */
  desc: string;
  /** CSV 表头（默认用 key） */
  header: string;
  generate: (faker: Faker, rowIndex: number, locale?: LocaleName) => string | number;
}

const pick = (faker: Faker, arr: readonly string[]): string =>
  faker.helpers.arrayElement(arr as unknown as string[]);

const MAJORS = [
  '计算机科学与技术',
  '软件工程',
  '数据科学与大数据技术',
  '网络工程',
  '信息安全',
  '电子信息工程',
] as const;

const GRADES = ['大一', '大二', '大三', '大四'] as const;

const COURSES = [
  '数据库原理',
  '操作系统',
  '计算机网络',
  '数据结构',
  '高等数学',
  '软件工程',
  '编译原理',
] as const;

export const FIELDS: FieldDef[] = [
  {
    key: 'id',
    label: 'ID（自增）',
    desc: '从 1 开始的自增整数，常作主键',
    header: 'id',
    generate: (_f, i) => i + 1,
  },
  {
    key: 'uuid',
    label: 'UUID',
    desc: '全局唯一标识',
    header: 'uuid',
    generate: (f) => f.string.uuid(),
  },
  {
    key: 'studentNo',
    label: '学号',
    desc: '形如 20261001 的学号',
    header: 'student_no',
    generate: (_f, i) => `2026${String(1001 + i).padStart(4, '0')}`,
  },
  {
    key: 'name',
    label: '姓名',
    desc: '按当前语言生成人名',
    header: 'name',
    generate: (f) => f.person.fullName(),
  },
  {
    key: 'gender',
    label: '性别',
    desc: '男 / 女',
    header: 'gender',
    generate: (f) => pick(f, ['男', '女']),
  },
  {
    key: 'age',
    label: '年龄',
    desc: '18~65 的整数',
    header: 'age',
    generate: (f) => f.number.int({ min: 18, max: 65 }),
  },
  {
    key: 'birthdate',
    label: '出生日期',
    desc: 'YYYY-MM-DD',
    header: 'birthdate',
    generate: (f) => f.date.birthdate({ min: 18, max: 65, mode: 'age' }).toISOString().slice(0, 10),
  },
  {
    key: 'major',
    label: '专业',
    desc: '从常见专业中随机取一个',
    header: 'major',
    generate: (f) => pick(f, MAJORS),
  },
  {
    key: 'grade',
    label: '年级',
    desc: '大一 ~ 大四',
    header: 'grade',
    generate: (f) => pick(f, GRADES),
  },
  {
    key: 'course',
    label: '课程',
    desc: '从常见课程中随机取一门',
    header: 'course',
    generate: (f) => pick(f, COURSES),
  },
  {
    key: 'score',
    label: '成绩',
    desc: '0~100 的整数',
    header: 'score',
    generate: (f) => f.number.int({ min: 0, max: 100 }),
  },
  {
    key: 'username',
    label: '用户名',
    desc: '英文字母用户名',
    header: 'username',
    generate: (f) => f.internet.username(),
  },
  {
    key: 'email',
    label: '邮箱',
    desc: '随机邮箱地址',
    header: 'email',
    generate: (f) => f.internet.email().toLowerCase(),
  },
  {
    key: 'phone',
    label: '电话',
    desc: '按当前语言生成电话号码',
    header: 'phone',
    generate: (f) => f.phone.number(),
  },
  {
    key: 'province',
    label: '省份',
    desc: '按当前语言生成省份/州',
    header: 'province',
    generate: (f) => f.location.state(),
  },
  {
    key: 'city',
    label: '城市',
    desc: '按当前语言生成城市',
    header: 'city',
    generate: (f) => f.location.city(),
  },
  {
    key: 'address',
    label: '地址',
    desc: '街道地址',
    header: 'address',
    generate: (f) => f.location.streetAddress(),
  },
  {
    key: 'zip',
    label: '邮编',
    desc: '邮政编码',
    header: 'zip',
    generate: (f) => f.location.zipCode(),
  },
  {
    key: 'company',
    label: '公司',
    desc: '随机公司名',
    header: 'company',
    generate: (f) => f.company.name(),
  },
  {
    key: 'job',
    label: '职业',
    desc: '随机职位名称',
    header: 'job',
    generate: (f) => f.person.jobTitle(),
  },
  {
    key: 'product',
    label: '商品',
    desc: '随机商品名',
    header: 'product',
    generate: (f) => f.commerce.productName(),
  },
  {
    key: 'price',
    label: '价格',
    desc: '10~5000，保留 2 位小数',
    header: 'price',
    generate: (f) => f.commerce.price({ min: 10, max: 5000, dec: 2 }),
  },
  {
    key: 'date',
    label: '日期',
    desc: '过去某天的 YYYY-MM-DD',
    header: 'date',
    generate: (f) => f.date.past().toISOString().slice(0, 10),
  },
  {
    key: 'datetime',
    label: '日期时间',
    desc: 'YYYY-MM-DD HH:mm:ss',
    header: 'datetime',
    generate: (f) => f.date.recent().toISOString().replace('T', ' ').slice(0, 19),
  },
  {
    key: 'ip',
    label: 'IP 地址',
    desc: '随机 IPv4/v6',
    header: 'ip',
    generate: (f) => f.internet.ip(),
  },
  {
    key: 'url',
    label: '网址',
    desc: '随机 URL',
    header: 'url',
    generate: (f) => f.internet.url(),
  },
  {
    key: 'active',
    label: '是否有效',
    desc: '是 / 否（英文模式为 Yes / No）',
    header: 'active',
    generate: (f, _i, locale) => {
      const yes = f.datatype.boolean();
      return locale === 'en' ? (yes ? 'Yes' : 'No') : yes ? '是' : '否';
    },
  },
  {
    key: 'note',
    label: '备注',
    desc: '一句随机文本',
    header: 'note',
    generate: (f) => f.lorem.sentence(),
  },
];

export const FIELD_MAP: Record<FieldKey, FieldDef> = Object.fromEntries(
  FIELDS.map((f) => [f.key, f]),
) as Record<FieldKey, FieldDef>;

export function isFieldKey(s: string): s is FieldKey {
  return s in FIELD_MAP;
}

/** 解析 `--fields a,b,c`，返回合法字段并收集非法字段名 */
export function parseFieldList(input: string): { fields: FieldKey[]; invalid: string[] } {
  const fields: FieldKey[] = [];
  const invalid: string[] = [];
  for (const part of input.split(',')) {
    const k = part.trim();
    if (!k) continue;
    if (isFieldKey(k)) {
      if (!fields.includes(k)) fields.push(k);
    } else {
      invalid.push(k);
    }
  }
  return { fields, invalid };
}
