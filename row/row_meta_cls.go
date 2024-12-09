package row

import (
	"csdb-teach/cfs"
	"csdb-teach/conf"
	"encoding/binary"
	"errors"
	"math"
	"strings"
)

type Meta struct {
	tp     uint8
	attr   uint8
	DbId   uint8
	tbId   uint32
	colId  uint32
	mType  uint16
	unused uint16
	nl     uint8
	data   []byte
}

func NewEmptyMeta() *Meta {
	var meta = new(Meta)
	return meta
}

func NewMetaRow(tp, attr, dbId uint8, tbId, colId uint32, mType uint16, value string) (*Meta, error) {
	var nl = len(value)
	if nl > math.MaxUint8 {
		return nil, errors.New(conf.ErrNameTooLong)
	}
	var meta = new(Meta)
	meta.tp = tp
	meta.attr = attr | conf.AttrExists
	meta.DbId = dbId
	meta.tbId = tbId
	meta.colId = colId
	meta.mType = mType
	meta.unused = 0
	meta.nl = uint8(len(value))
	meta.data = make([]byte, meta.nl)
	copy(meta.data, strings.ToUpper(value))
	return meta, nil
}

func (m *Meta) Read(data []byte) *Meta {
	m.tp = data[0]
	m.attr = data[1]
	m.DbId = data[2]
	m.tbId = binary.BigEndian.Uint32(data[3:7])
	m.tbId = binary.BigEndian.Uint32(data[7:11])
	m.mType = binary.BigEndian.Uint16(data[11:13])
	m.unused = binary.BigEndian.Uint16(data[13:15])
	m.nl = data[15]
	m.data = make([]byte, m.nl)
	copy(m.data, data[16:])
	return m
}

func (m *Meta) Encode() []byte {
	var data = make([]byte, conf.RowHeaderSize+m.nl)
	data[0] = m.tp
	data[1] = m.attr
	data[2] = m.DbId
	binary.BigEndian.PutUint32(data[3:7], m.tbId)
	binary.BigEndian.PutUint32(data[7:11], m.tbId)
	binary.BigEndian.PutUint16(data[11:15], m.mType)
	binary.BigEndian.PutUint16(data[11:15], m.unused)
	data[15] = m.nl
	copy(data[16:], m.data)
	return data
}

func (m *Meta) Clean() {
	m.tp = 0
	m.attr = 0xFF & m.attr
	m.DbId = 0
	m.tbId = 0
	m.colId = 0
	m.nl = 0
	m.data = nil
}

func (m *Meta) Decode(page *cfs.Page, offset int64) *Meta {
	m.Read(page.Raw()[offset:])
	return m
}

func (m *Meta) String() string {
	return string(m.data)
}
