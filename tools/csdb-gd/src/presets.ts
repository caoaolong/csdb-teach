import type { FieldKey } from './fields.js';

export interface Preset {
  key: string;
  label: string;
  desc: string;
  fields: FieldKey[];
}

export const PRESETS: Preset[] = [
  {
    key: 'student',
    label: '学生表',
    desc: '学号 / 姓名 / 性别 / 专业 / 年级 / 课程 / 成绩…',
    fields: ['id', 'studentNo', 'name', 'gender', 'age', 'major', 'grade', 'course', 'score', 'email', 'phone'],
  },
  {
    key: 'user',
    label: '用户表',
    desc: '用户名 / 姓名 / 邮箱 / 电话 / 城市 / 地址…',
    fields: ['id', 'uuid', 'username', 'name', 'gender', 'age', 'email', 'phone', 'city', 'address', 'date'],
  },
  {
    key: 'employee',
    label: '员工表',
    desc: '姓名 / 公司 / 职业 / 电话 / 城市…',
    fields: ['id', 'name', 'gender', 'age', 'company', 'job', 'email', 'phone', 'province', 'city', 'date'],
  },
  {
    key: 'order',
    label: '订单表',
    desc: '订单号 / 用户 / 商品 / 价格 / 日期 / 地址…',
    fields: ['id', 'uuid', 'username', 'product', 'price', 'phone', 'address', 'date', 'datetime', 'active', 'note'],
  },
  {
    key: 'custom',
    label: '自定义',
    desc: '不使用预设，手动勾选全部字段',
    fields: [],
  },
];

export function getPreset(key: string): Preset | undefined {
  return PRESETS.find((p) => p.key === key);
}
