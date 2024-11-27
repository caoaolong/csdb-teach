package cfs

import (
	"testing"
)

func TestCreate(t *testing.T) {
	pf := new(PageFile)
	err := pf.Create("test1")
	if err != nil {
		t.Fatal(err)
	}
	for i := 0; i < 10; i++ {
		err = pf.AppendPage(uint16(i), 0, 0)
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
		err = page.Write(pf, []byte("Test,CS.DB!"), false)
		if err != nil {
			t.Fatal(err)
		}
	}
	err = pf.Close()
	if err != nil {
		t.Fatal(err)
	}
}
