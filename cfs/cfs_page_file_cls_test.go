package cfs

import "testing"

func TestCreate(t *testing.T) {
	pf := new(PageFile)
	err := pf.Create("test1")
	if err != nil {
		t.Fatal(err)
	}
	_ = pf.Close()
}

func TestRead(t *testing.T) {
	pf := new(PageFile)
	err := pf.Read("test1")
	if err != nil {
		t.Fatal(err)
	}
	_ = pf.Close()
}
