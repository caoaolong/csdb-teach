package cfs

import (
	"csdb-teach/conf"
	"errors"
	"fmt"
	"golang.org/x/sys/windows"
	"os"
	"path/filepath"
	"strings"
	"syscall"
)

type Fs struct {
	fsType      string
	fsFreeSize  uint64
	fsTotalSize uint64
}

type PageFile struct {
	tempName     string
	originalName string
	fp           *os.File
	fi           os.FileInfo
	pageCount    uint16
	last         uint16
	pages        []*Page
	dirty        bool
	maxPageCount int
	fs           Fs
}

func (pf *PageFile) IsDirty() bool {
	return pf.dirty
}

func (pf *PageFile) freeSize() error {
	var totalNumberOfBytes uint64
	var totalNumberOfFreeBytes uint64
	err := windows.GetDiskFreeSpaceEx(nil, nil, &totalNumberOfBytes, &totalNumberOfFreeBytes)
	if err != nil {
		return err
	}
	pf.fs.fsTotalSize = totalNumberOfBytes
	pf.fs.fsFreeSize = totalNumberOfFreeBytes
	return nil
}

func (pf *PageFile) checkFs() error {
	ap, err := filepath.Abs(pf.originalName)
	if err != nil {
		return err
	}
	ptr, err := syscall.UTF16PtrFromString(ap[:3])
	if err != nil {
		return err
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
		return err
	}

	var fsType = syscall.UTF16ToString(fsName[:])
	pf.fs.fsType = fsType
	pf.maxPageCount = conf.FsMaxPageCount[fsType]
	return nil
}

func (pf *PageFile) checkAppend() error {
	var newSize = int(pf.pageCount+1)*conf.FilePageSize + conf.FileHeaderSize
	// 检查磁盘空间是否用完
	if uint64(newSize) > pf.fs.fsFreeSize {
		return errors.New(conf.ErrPageFileFull)
	}
	// 检查现有空间是否用完
	if int64(newSize) <= pf.fi.Size() {
		return nil
	}
	return pf.expand()
}

func (pf *PageFile) expand() error {
	if pf.pageCount == 0 {
		pf.pageCount = 1
	}
	var newSize = pf.pageCount * 2
	if int(newSize) > pf.maxPageCount {
		return errors.New(conf.ErrPageFileFull)
	} else {
		err := pf.fp.Truncate(int64(newSize) + conf.FileHeaderSize)
		if err != nil {
			return err
		}
		pf.pageCount = newSize
		return nil
	}
}

func (pf *PageFile) Open(filename string) error {
	err := pf.Create(filename)
	if err != nil {
		if os.IsExist(err) {
			err = pf.Read(filename)
			if err != nil {
				return err
			}
		}
	}
	return pf.Flush()
}

func (pf *PageFile) Create(filename string) error {
	err := pf.checkFs()
	if err != nil {
		return err
	}
	err = pf.freeSize()
	if err != nil {
		return err
	}
	var originalName = fmt.Sprintf("%s/%s.cs", conf.Workspace, filename)
	var tempName = fmt.Sprintf("%s/%s.cs.tmp", conf.Workspace, conf.RandomInt(5))
	pf.originalName = originalName
	pf.tempName = tempName
	// 创建文件
	fp, err := os.OpenFile(tempName, os.O_RDWR|os.O_CREATE|os.O_EXCL, 0644)
	if err != nil {
		return err
	}
	pf.fp = fp
	// 获取文件属性
	pf.fi, err = fp.Stat()
	if err != nil {
		return err
	}
	// 设置文件大小
	err = pf.fp.Truncate(int64(conf.FilePageSize*conf.FilePageInitCount + conf.FileHeaderSize))
	if err != nil {
		return err
	}
	// 设置文件头
	_, err = pf.fp.Write([]byte(conf.FileHeaderMagic))
	pf.pageCount = uint16(conf.FilePageInitCount)
	pf.pages = make([]*Page, conf.FilePageInitCount)
	pf.dirty = true
	return err
}

func (pf *PageFile) Read(filename string) error {
	// TODO: 处理并发读写问题
	var originalName = fmt.Sprintf("%s/%s.cs", conf.Workspace, filename)
	pf.originalName = originalName
	// 打开文件
	fp, err := os.OpenFile(originalName, os.O_RDWR, 0644)
	if err != nil {
		return err
	}
	pf.fp = fp
	// 获取文件属性
	pf.fi, err = fp.Stat()
	if err != nil {
		return err
	}
	v := (pf.fi.Size() - int64(conf.FileHeaderSize)) % int64(conf.FilePageSize)
	if v > 0 {
		return errors.New(conf.ErrFileFormat)
	}
	pf.pages = make([]*Page, (pf.fi.Size()-int64(conf.FileHeaderSize))/int64(conf.FilePageSize))
	var header = make([]byte, conf.FileHeaderSize)
	_, err = pf.fp.Read(header)
	if err != nil {
		return err
	}
	// 判断文件头
	if strings.Compare(string(header[:len(conf.FileHeaderMagic)]), conf.FileHeaderMagic) != 0 {
		return errors.New(conf.ErrFileFormat)
	}
	pf.pageCount = uint16((pf.fi.Size() - conf.FileHeaderSize) / int64(conf.FilePageSize))
	for index := int64(conf.FileHeaderSize); index < pf.fi.Size(); index += int64(conf.FilePageSize) {
		// 读取数据
		var page = NewEmptyPage(index)
		err = page.Read(pf, false)
		if err != nil {
			return err
		}
		pf.pages[index/int64(conf.FilePageSize)] = page
		if !page.IsExists() {
			break
		}
	}
	return err
}

func (pf *PageFile) Flush() error {
	for _, e := range pf.pages {
		if e != nil && e.IsDirty() {
			err := e.Write(pf, e.data, true)
			if err != nil {
				return err
			}
		}
	}
	if pf.dirty {
		err := pf.fp.Sync()
		if err != nil {
			return err
		}
		pf.dirty = false
		return pf.freeSize()
	} else {
		return nil
	}
}

func (pf *PageFile) Close() error {
	err := pf.Flush()
	if err != nil {
		return err
	}
	err = pf.fp.Close()
	if err != nil {
		return err
	}
	if pf.tempName != "" && pf.originalName != "" {
		return os.Rename(pf.tempName, pf.originalName)
	}
	return err
}

func (pf *PageFile) Page(index int, body bool) (*Page, error) {
	if index < 0 || index >= int(pf.pageCount) {
		return nil, errors.New(conf.ErrPageIndex)
	}
	pf.pages[index].offset = int64(index*conf.FilePageSize) + conf.FileHeaderSize
	return pf.pages[index], pf.pages[index].Read(pf, body)
}

func (pf *PageFile) PageByType(pType uint8, dbId uint8) (*Page, error) {
	var err error
	for i := 0; i < int(pf.pageCount); i++ {
		if pf.pages[i] == nil {
			pf.pages[i] = NewEmptyPage(conf.FileHeaderSize + int64(conf.PageHeaderSize*i))
			pf.pages[i], err = pf.Page(i, true)
			if err != nil {
				return nil, err
			}
		}
		page := pf.pages[i]
		if page.Type() == 0 {
			err = page.Read(pf, true)
			if err != nil {
				return nil, err
			}
			page.Attr(pType)
			page.DBId(dbId)
			return page, nil
		}
		if pf.pages[i].Type() == pType {
			return page, page.Read(pf, true)
		}
	}
	return pf.AppendPage(pf.pageCount, pType, dbId)
}
