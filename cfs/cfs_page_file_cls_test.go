package cfs

import (
	"fmt"
	"golang.org/x/sys/windows"
	"path/filepath"
	"syscall"
	"testing"
)

func TestCreate(t *testing.T) {
	pf := new(PageFile)
	err := pf.Create("test1")
	if err != nil {
		t.Fatal(err)
	}
	for i := 0; i < 10; i++ {
		_, err = pf.AppendPage(uint16(i), 0, 0)
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

func TestFileSystem(t *testing.T) {
	p := "../cs/test1.cs"
	p, err := filepath.Abs(p)
	if err != nil {
		t.Fatal(err)
	}
	//p := "F:/"
	t.Log(p[:3])
	ptr, err := syscall.UTF16PtrFromString(p[:3])
	if err != nil {
		t.Fatal(err)
	}
	var volumeName [syscall.MAX_PATH + 1]uint16
	var fsName [syscall.MAX_PATH + 1]uint16
	var serialNumber, maxComponentLen, fileSystemFlags uint32
	// Get volume information
	err = windows.GetVolumeInformation(
		ptr,
		&volumeName[0],
		uint32(len(volumeName)),
		&serialNumber,
		&maxComponentLen,
		&fileSystemFlags,
		&fsName[0],
		uint32(len(fsName)),
	)
	if err != nil {
		t.Fatal(err)
	}

	// Convert the file system name to a Go string
	vName := syscall.UTF16ToString(volumeName[:])
	fsType := syscall.UTF16ToString(fsName[:])
	t.Logf("%s : %s\n", vName, fsType)
}

func TestVolumeSpace(t *testing.T) {

	var totalNumberOfBytes uint64
	var totalNumberOfFreeBytes uint64
	err := windows.GetDiskFreeSpaceEx(nil, nil, &totalNumberOfBytes, &totalNumberOfFreeBytes)
	if err != nil {
		t.Fatal(err)
	}
	fmt.Println("totalNumberOfBytes", totalNumberOfBytes/1024/1024/1024)
	fmt.Println("totalNumberOfFreeBytes", totalNumberOfFreeBytes/1024/1024/1024)
}
