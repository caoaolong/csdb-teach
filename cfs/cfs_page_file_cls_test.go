package cfs

import (
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
		err = pf.AppendPage(uint16(i), 0, 1, uint32(i+1), []byte(fmt.Sprintf("Hello,%d", i+1)))
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

func TestPage(t *testing.T) {
	pf := new(PageFile)
	err := pf.Read("test1")
	if err != nil {
		t.Fatal(err)
	}
	page, err := pf.Page(1, false)
	if err != nil {
		t.Fatal(err)
	}
	if page.IsEmpty() && page.IsExists() {
		err = page.Read(pf, true)
		if err != nil {
			t.Fatal(err)
		}
		page.Data([]byte("Test,CS.DB!"))
		err = page.Write(pf)
		if err != nil {
			t.Fatal(err)
		}
	}
	err = pf.Close()
	if err != nil {
		t.Fatal(err)
	}
}
