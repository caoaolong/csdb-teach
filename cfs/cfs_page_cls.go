package cfs

import (
	"csdb-teach/conf"
	"encoding/binary"
	"io"
)

type Page struct {
	attr     uint8
	parentId uint16
	ownerId  uint16
	dbId     uint8
	tbId     uint32
	unused   [6]byte
	data     []byte
}

func (p *Page) IsInMemory() bool {
	return (p.attr & conf.AttrInMemory) > 0
}

func (p *Page) IsExists() bool {
	return (p.attr & conf.AttrExists) > 0
}

func (p *Page) IsData() bool {
	return (p.attr & conf.AttrData) > 0
}

func (p *Page) IsStructure() bool {
	return (p.attr & conf.AttrStructure) > 0
}

func (p *Page) IsString() bool {
	return (p.attr & conf.AttrString) > 0
}

func (p *Page) write(pf *PageFile, data []byte) error {
	// 定位写入位置
	_, err := pf.fp.Seek(int64(conf.FileHeaderSize)+int64(+conf.FilePageSize)*int64(p.ownerId-1), io.SeekStart)
	if err != nil {
		return err
	}
	var header = make([]byte, conf.FileHeaderSize)
	header[0] = p.attr
	binary.BigEndian.PutUint16(header[1:3], p.parentId)
	binary.BigEndian.PutUint16(header[3:5], p.ownerId)
	header[5] = p.dbId
	binary.BigEndian.PutUint32(header[6:10], p.tbId)
	_, err = pf.fp.Write(header)
	if err != nil {
		return err
	}
	p.data = make([]byte, conf.FilePageSize-conf.PageHeaderSize)
	copy(p.data, data)
	_, err = pf.fp.Write(p.data)
	return err
}

func (p *Page) read(pf *PageFile, offset int64) error {
	var data = make([]byte, conf.FilePageSize)
	_, err := pf.fp.Seek(offset, io.SeekStart)
	if err != nil {
		return err
	}
	_, err = pf.fp.Read(data)
	if err != nil {
		return err
	}
	p.attr = data[0]
	p.parentId = binary.BigEndian.Uint16(data[1:3])
	p.ownerId = binary.BigEndian.Uint16(data[3:5])
	p.dbId = data[5]
	p.tbId = binary.BigEndian.Uint32(data[6:10])
	// 页存在且在内存中则读取并保存页的数据
	if p.IsExists() && p.IsInMemory() {
		p.data = make([]byte, conf.FilePageSize-conf.PageHeaderSize)
		copy(p.data, data[conf.PageHeaderSize:])
	}
	return err
}

func (p *Page) clear(pf *PageFile) error {
	_, err := pf.fp.Seek(int64(conf.FileHeaderSize)+int64(+conf.FilePageSize)*int64(p.ownerId-1), io.SeekStart)
	if err != nil {
		return err
	}
	var data = make([]byte, conf.FilePageSize-conf.FileHeaderSize)
	_, err = pf.fp.Write(data)
	return err
}

func (pf *PageFile) AppendPage(parentId uint16, attr uint8, db uint8, tb uint32, data []byte) error {
	// TODO: 判断是否可以追加
	// 初始化 Page
	var page = new(Page)
	pf.pages[pf.pageCount] = page
	pf.pageCount++
	page.ownerId = pf.pageCount
	if parentId > 0 {
		page.parentId = parentId
	} else if parentId == 0 && pf.pageCount > 1 {
		page.parentId = pf.pageCount - 1
	} else {
		page.parentId = 0
	}
	page.attr = conf.AttrExists | attr
	page.dbId = db
	page.tbId = tb
	// 写入 Page
	return page.write(pf, data)
}

func (pf *PageFile) ClearPage(index uint16) error {
	return pf.pages[index].clear(pf)
}
