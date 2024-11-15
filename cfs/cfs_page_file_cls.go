package cfs

import (
	"csdb-teach/conf"
	"errors"
	"fmt"
	"os"
	"strings"
)

type PageFile struct {
	tempName     string
	originalName string
	fp           *os.File
	fi           os.FileInfo
}

func (pf *PageFile) Create(filename string) error {
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
	return pf.parse()
}

func (pf *PageFile) parse() error {
	var header = make([]byte, conf.FileHeaderSize)
	_, err := pf.fp.Read(header)
	if err != nil {
		return err
	}
	// 判断文件头
	if strings.Compare(string(header[:len(conf.FileHeaderMagic)]), conf.FileHeaderMagic) != 0 {
		return errors.New(conf.ErrFileFormat)
	}
	// TODO: 解析页文件的每一项
	return err
}

func (pf *PageFile) Flush() error {
	return pf.fp.Sync()
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
	return os.Rename(pf.tempName, pf.originalName)
}
