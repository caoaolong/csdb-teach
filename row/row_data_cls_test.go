package row

import (
	"csdb-teach/cfs"
	"csdb-teach/conf"
	"fmt"
	"testing"
)

func TestWriteDataRow(t *testing.T) {
	pf := new(cfs.PageFile)
	err := pf.Read("test1")
	if err != nil {
		t.Fatal(err)
	}
	page, err := pf.Page(0, true)
	if err != nil {
		t.Fatal(err)
	}
	classId, _ := NewDataValue(conf.DvNumber, int64(1))
	className, _ := NewDataValue(conf.DvString, []byte("高三（2）班"))
	var dataBytes = make([]byte, 0)
	dataBytes = append(dataBytes, DataValueBytes(classId)...)
	dataBytes = append(dataBytes, DataValueBytes(className)...)
	// 创建一个Data行
	data := NewDataRow(1, 1, 2, 1, dataBytes)
	// 写入行
	err = page.Write(pf, data.Encode(), false)
	if err != nil {
		t.Fatal(err)
	}
	err = pf.Close()
	if err != nil {
		t.Fatal(err)
	}
}

func TestReadDataRow(t *testing.T) {
	pf := new(cfs.PageFile)
	err := pf.Read("test1")
	if err != nil {
		t.Fatal(err)
	}
	page, err := pf.Page(0, true)
	fmt.Println(NewEmptyData().Decode(page, 0).String())
	err = pf.Close()
	if err != nil {
		t.Fatal(err)
	}
}
