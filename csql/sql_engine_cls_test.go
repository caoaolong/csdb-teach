package csql

import (
	"fmt"
	"strings"
	"testing"
)

func TestSqlEngine(t *testing.T) {
	var script = "create database school;"
	var se = NewSqlEngine()
	se.ParseToken(script)
	for _, e := range se.Tokens() {
		t.Logf("Token: %s(%d, %d)\n", e.Value, e.Type, e.OpType)
	}
	se.ParseSyntax()
	for _, e := range se.Entries() {
		var line strings.Builder
		for _, tk := range e {
			line.WriteString(fmt.Sprintf("%s(%d, %d) ", tk.Value, tk.Type, tk.OpType))
		}
		t.Log(line.String())
		line.Reset()
	}
}
