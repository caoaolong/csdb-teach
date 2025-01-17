package cfs

import (
	"csdb-teach/conf"
	"encoding/binary"
	"errors"
	"io"
	"strings"
)

type Page struct {
	// Coded fields
	attr     uint8
	parentId uint16
	ownerId  uint16
	dbId     uint8
	lOffset  uint16
	unused   [8]byte
	data     []byte
	// Non-coded fields
	offset  int64
	entries []int64
	dirty   bool
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

func (p *Page) IsDirty() bool {
	return p.dirty
}

func (p *Page) Attr(attr uint8) {
	p.attr |= attr
}

func (p *Page) DBId(dbId uint8) {
	p.dbId = dbId
}

func (p *Page) Raw() []byte {
	return p.data
}

func (p *Page) Offset() uint16 {
	return p.lOffset
}

func (p *Page) Index() uint16 {
	return uint16((p.offset - int64(conf.FileHeaderSize)) / int64(conf.FilePageSize))
}

func (p *Page) Cover(offset int64, data []byte) {
	copy(p.data[offset:], data)
	p.dirty = true
}

func (p *Page) WriteMemory(data []byte, overlay bool) {
	if p.data == nil {
		p.data = make([]byte, conf.FilePageSize-conf.PageHeaderSize)
	}
	if overlay {
		copy(p.data, data)
	} else {
		copy(p.data[p.lOffset:], data)
		p.lOffset += uint16(len(data))
		p.dirty = true
	}
}

func (p *Page) Write(pf *PageFile, data []byte, overlay bool) error {
	p.WriteMemory(data, overlay)
	if !p.IsDirty() {
		return nil
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
	binary.BigEndian.PutUint16(header[6:8], p.lOffset)
	_, err = pf.fp.Write(header)
	if err != nil {
		return err
	}
	_, err = pf.fp.Write(p.data)
	if err != nil {
		return err
	}
	pf.dirty = true
	p.dirty = false
	return nil
}

func (p *Page) Read(pf *PageFile, body bool) error {
	if p.IsDirty() {
		return nil
	}
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
	p.lOffset = binary.BigEndian.Uint16(data[6:8])
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

func (p *Page) Clear() {
	for i := 0; i < len(p.data); i++ {
		p.data[i] = 0
	}
	p.lOffset = 0
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

func (p *Page) FindRowByName(rowType uint8, value string) ([]byte, error) {
	if p.entries == nil {
		err := p.Scan()
		if err != nil {
			return nil, err
		}
	}
	value = strings.ToUpper(value)
	for _, offset := range p.entries {
		if conf.RowType(p.data[offset]) == rowType {
			var nl = int(p.data[offset+15])
			if value == string(p.data[offset+16:int(offset)+16+nl]) {
				return p.data[offset : int(offset)+conf.RowHeaderSize+nl], nil
			}
		}
	}
	return nil, errors.New(conf.ErrPageNotFound)
}

func (p *Page) FindRowByID(rowType uint8, id uint32) ([]byte, int64, error) {
	var found = false
	err := p.Scan()
	if err != nil {
		return nil, 0, err
	}
	for _, offset := range p.entries {
		if conf.RowType(p.data[offset]) == rowType {
			if rowType == conf.RowTypeTable {
				if id == binary.BigEndian.Uint32(p.data[offset+3:offset+7]) {
					found = true
				}
			} else if rowType == conf.RowTypeColumn {
				if id == binary.BigEndian.Uint32(p.data[offset+7:offset+11]) {
					found = true
				}
			} else if rowType == conf.RowTypeDatabase {
				if id == uint32(p.data[offset+2]) {
					found = true
				}
			}
			if found {
				return p.data[offset : int(offset)+conf.RowHeaderSize+int(p.data[offset+15])], offset, nil
			}
		}
	}
	return nil, 0, errors.New(conf.ErrPageNotFound)
}

func (pf *PageFile) AppendPage(parentId uint16, attr uint8, db uint8) (*Page, error) {
	if pf.last+1 == pf.pageCount {
		err := pf.checkAppend()
		if err != nil {
			return nil, err
		}
	}
	// 初始化 Page
	var page = NewEmptyPage(conf.FileHeaderSize + int64(parentId)*int64(conf.FilePageSize))
	pf.pages[pf.last] = page
	pf.last++
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
	err := page.Write(pf, []byte{}, true)
	if err != nil {
		return page, err
	}
	pf.dirty = true
	return page, nil
}
