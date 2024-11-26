package conf

import (
	"fmt"
	"testing"
)

func TestIDWare(t *testing.T) {
	InitIDFile()
	db, err := IDW.Database()
	if err != nil {
		t.Fatal(err)
	}
	tb, err := IDW.Table()
	if err != nil {
		t.Fatal(err)
	}
	col, err := IDW.Column()
	if err != nil {
		t.Fatal(err)
	}
	fmt.Printf("db: %d, tb: %d, col: %d\n", db, tb, col)
	err = CloseIDWare()
	if err != nil {
		t.Fatal(err)
	}
}
