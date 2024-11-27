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
	_, err = NewDatabase(pf, "school")
	if err != nil {
		t.Fatal(err)
	}
	err = pf.Close()
	if err != nil {
		t.Fatal(err)
	}
}
