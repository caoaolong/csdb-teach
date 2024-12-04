package cfs

import (
	"csdb-teach/conf"
	"encoding/binary"
	"errors"
	"io"
)

type Page struct {
	// Coded fields
	offset   int64
	attr     uint8
	parentId uint16
	ownerId  uint16
	dbId     uint8
	lOffset  uint32
	unused   [6]byte
	data     []byte
	// Non-coded fields
	entries []int64
}

func NewEmptyPage(offset int64) *Page {
	var page = new(Page)
	page.offset = offset
	return page
}

func (p *Page) IsExists() bool {
	return (p.attr & conf.AttrExists) > 0
}

func (p *Page) Type() uint8 {
	return p.attr & conf.PageTypeMask
}

func (p *Page) IsEmpty() bool {
	return p.data == nil
}

func (p *Page) Attr(pf *PageFile, attr uint8) error {
	p.attr |= attr
	// 定位写入位置
	_, err := pf.fp.Seek(p.offset, io.SeekStart)
	if err != nil {
		return err
	}
	_, err = pf.fp.Write([]byte{p.attr})
	if err != nil {
		return err
	}
	pf.dirty = true
	return nil
}

func (p *Page) Raw() []byte {
	return p.data
}

func (p *Page) Write(pf *PageFile, data []byte, overlay bool) error {
	if p.data == nil {
		p.data = make([]byte, conf.FilePageSize-conf.PageHeaderSize)
	}
	if overlay {
		copy(p.data, data)
	} else {
		copy(p.data[p.lOffset:], data)
		p.lOffset += uint32(len(data))
	}
	// 定位写入位置
	_, err := pf.fp.Seek(p.offset, io.SeekStart)
	if err != nil {
		return err
	}
	var header = make([]byte, conf.FileHeaderSize)
	header[0] = p.attr
	binary.BigEndian.PutUint16(header[1:3], p.parentId)
	binary.BigEndian.PutUint16(header[3:5], p.ownerId)
	header[5] = p.dbId
	binary.BigEndian.PutUint32(header[6:10], p.lOffset)
	_, err = pf.fp.Write(header)
	if err != nil {
		return err
	}
	_, err = pf.fp.Write(p.data)
	pf.dirty = true
	return err
}

func (p *Page) Read(pf *PageFile, body bool) error {
	var data = make([]byte, conf.FilePageSize)
	_, err := pf.fp.Seek(p.offset, io.SeekStart)
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
	p.lOffset = binary.BigEndian.Uint32(data[6:10])
	if body {
		p.data = make([]byte, conf.FilePageSize-conf.PageHeaderSize)
		copy(p.data, data[conf.PageHeaderSize:])
		err = p.Scan()
		if err != nil {
			return err
		}
	}
	return nil
}

func (p *Page) Clear(pf *PageFile) error {
	_, err := pf.fp.Seek(int64(conf.FileHeaderSize)+int64(+conf.FilePageSize)*int64(p.ownerId-1), io.SeekStart)
	if err != nil {
		return err
	}
	var data = make([]byte, conf.FilePageSize-conf.FileHeaderSize)
	_, err = pf.fp.Write(data)
	return err
}

func (p *Page) Scan() error {
	// TODO: 需要后续完善
	for offset := 0; offset < len(p.data); {
		switch conf.RowType(p.data[offset]) {
		case conf.RowTypeDatabase, conf.RowTypeTable, conf.RowTypeColumn:
			var nl = int(p.data[offset+15])
			p.entries = append(p.entries, int64(offset))
			offset += nl + conf.RowHeaderSize
			break
		case conf.RowTypeNull:
			return errors.New(conf.ErrRowType)
		case conf.RowTypeUnknown:
			offset = len(p.data)
			break
		}
	}
	return nil
}

func (pf *PageFile) AppendPage(parentId uint16, attr uint8, db uint8) (*Page, error) {
	err := pf.checkAppend()
	if err != nil {
		return nil, err
	}
	// 初始化 Page
	var page = NewEmptyPage(conf.FileHeaderSize + int64(parentId)*int64(conf.FilePageSize))
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
	page.lOffset = 0
	// 写入 Page
	err = page.Write(pf, []byte{}, true)
	if err != nil {
		return page, err
	}
	pf.dirty = true
	return page, nil
}

func (pf *PageFile) ClearPage(index uint16) error {
	return pf.pages[index].Clear(pf)
}
