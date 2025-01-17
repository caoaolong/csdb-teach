package row

import (
	"csdb-teach/cfs"
	"csdb-teach/conf"
	"fmt"
	"testing"
)

func TestCreateIndex(t *testing.T) {
	var pf = new(cfs.PageFile)
	err := pf.Read("test1")
	if err != nil {
		t.Fatal(err)
	}
	page, err := pf.PageByType(conf.PageTypeIndex, 1)
	if err != nil {
		t.Fatal(err)
	}
	page.Clear()
	nums := []uint64{10, 20, 30, 40, 50, 25}
	for _, n := range nums {
		idx := NewIndex(page.Index(), page.Offset(), n)
		page.WriteMemory(idx.Encode(), false)
	}
	err = pf.Close()
	if err != nil {
		t.Fatal(err)
	}
}

func TestReadIndex(t *testing.T) {
	var pf = new(cfs.PageFile)
	err := pf.Read("test1")
	if err != nil {
		t.Fatal(err)
	}
	page, err := pf.PageByType(conf.PageTypeIndex, 1)
	if err != nil {
		t.Fatal(err)
	}
	data := page.Raw()
	for i := 0; ; i += conf.IndexRowSize {
		idx := NewEmptyIndex().Read(data[i : i+conf.IndexRowSize])
		if idx.Attr == 0 {
			break
		}
		fmt.Println(idx.Value)
	}
	err = pf.Close()
	if err != nil {
		t.Fatal(err)
	}
}
