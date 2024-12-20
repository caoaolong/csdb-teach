package row

import (
	"csdb-teach/cfs"
	"csdb-teach/conf"
	"fmt"
	"testing"
)

func TestWriteMetaRow(t *testing.T) {
	pf := new(cfs.PageFile)
	err := pf.Read("test1")
	if err != nil {
		t.Fatal(err)
	}
	page, err := pf.Page(1, true)
	// 创建一个Meta行
	meta, err := NewMetaRow(conf.RowTypeDatabase, 0, 1, 0, 0, 0, "mysql")
	if err != nil {
		t.Fatal(err)
	}
	// 写入行
	err = page.Write(pf, meta.Encode(), false)
	if err != nil {
		t.Fatal(err)
	}
	err = pf.Close()
	if err != nil {
		t.Fatal(err)
	}
}

func TestReadMetaRow(t *testing.T) {
	pf := new(cfs.PageFile)
	err := pf.Read("test1")
	if err != nil {
		t.Fatal(err)
	}
	page, err := pf.Page(1, true)
	fmt.Println(NewEmptyMeta().Decode(page, 0).String())
	err = pf.Close()
	if err != nil {
		t.Fatal(err)
	}
}
