package row

import (
	"csdb-teach/cfs"
	"csdb-teach/conf"
	"testing"
)

func TestMetaRow(t *testing.T) {
	pf := new(cfs.PageFile)
	err := pf.Read("test1")
	if err != nil {
		t.Fatal(err)
	}
	page, err := pf.Page(1, true)
	// 创建一个Meta行
	meta, err := NewMetaRow(conf.RowTypeDatabase, 0, 1, 0, 0, "mysql")
	if err != nil {
		t.Fatal(err)
	}
	// 写入行
	page.Data(meta.Encode())
	err = page.Write(pf)
	if err != nil {
		t.Fatal(err)
	}
	err = pf.Close()
	if err != nil {
		t.Fatal(err)
	}
}
