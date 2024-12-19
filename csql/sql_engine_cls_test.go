package csql

import (
	"testing"
)

var script = `
use school;
insert into class(class_id, class_name) values(1, '高二（1）班');
`

func TestSqlEngine(t *testing.T) {

	// 创建SQL引擎
	var se = NewSqlEngine()

	// SQL编译器 - 前端
	// 词法解析
	se.ParseToken(script)
	// 语法解析
	ast, err := se.ParseSyntax()
	if err != nil {
		t.Fatal(err)
	}

	// SQL编译器 - 后端
	// 编译
	instructions, err := se.Compile(ast)
	if err != nil {
		t.Fatal(err)
	}
	// 运行
	err = se.Run(instructions)
	if err != nil {
		t.Fatal(err)
	}

	// 关闭引擎
	se.Close()
}
