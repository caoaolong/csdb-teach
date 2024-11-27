package cds

import (
	"csdb-teach/cfs"
	"csdb-teach/conf"
	"testing"
)

func TestNewDatabase(t *testing.T) {
	conf.InitIDFile()
	pf := new(cfs.PageFile)
	err := pf.Read("test1")
	if err != nil {
		t.Fatal(err)
	}
	db, err := NewDatabase(pf, "school")
	if err != nil {
		t.Fatal(err)
	}
	table, err := NewTable(pf, db, "student")
	if err != nil {
		t.Fatal(err)
	}
	_, err = NewColumn(pf, db, table, "std_name", conf.ColumnTypeNchar)
	if err != nil {
		t.Fatal(err)
	}
	_, err = NewColumn(pf, db, table, "std_age", conf.ColumnTypeTinyInt)
	if err != nil {
		t.Fatal(err)
	}
	_, err = NewColumn(pf, db, table, "std_sex", conf.ColumnTypeBit)
	if err != nil {
		t.Fatal(err)
	}
	err = pf.Close()
	if err != nil {
		t.Fatal(err)
	}
}
