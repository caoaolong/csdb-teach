package cfs

import (
	"csdb-teach/conf"
	"fmt"
	"testing"
)

func TestCreate(t *testing.T) {
	pf := new(PageFile)
	err := pf.Create("test1")
	if err != nil {
		t.Fatal(err)
	}
	for i := 0; i < 10; i++ {
		err = pf.AppendPage(uint16(i), conf.AttrInMemory, 1, uint32(i), []byte(fmt.Sprintf("Hello,%d", i+1)))
		if err != nil {
			t.Fatal(err)
		}
	}
	err = pf.Flush()
	if err != nil {
		t.Fatal(err)
	}
	err = pf.Close()
	if err != nil {
		t.Fatal(err)
	}
}

func TestRead(t *testing.T) {
	pf := new(PageFile)
	err := pf.Read("test1")
	if err != nil {
		t.Fatal(err)
	}
	err = pf.Close()
	if err != nil {
		t.Fatal(err)
	}
}
