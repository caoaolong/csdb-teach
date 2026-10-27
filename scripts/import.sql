DROP TABLE IF EXISTS students;
CREATE TABLE students(
  id INTEGER PRIMARY KEY,
  student_no TEXT,
  name TEXT,
  gender TEXT,
  age INTEGER,
  major TEXT,
  grade TEXT,
  course TEXT,
  score INTEGER,
  email TEXT,
  phone TEXT
);
.mode csv
.import --skip 1 build/students.csv students
SELECT COUNT(*) AS rows_imported FROM students;