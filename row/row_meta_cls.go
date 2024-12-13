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
	TbId   uint32
	ColId  uint32
	MType  uint16
	Length uint8
	Bind   uint8
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
	meta.TbId = tbId
	meta.ColId = colId
	meta.MType = mType
	meta.Length = 0
	meta.Bind = 0
	meta.nl = uint8(len(value))
	meta.data = make([]byte, meta.nl)
	copy(meta.data, strings.ToUpper(value))
	return meta, nil
}

func (m *Meta) Read(data []byte) *Meta {
	m.tp = data[0]
	m.attr = data[1]
	m.DbId = data[2]
	m.TbId = binary.BigEndian.Uint32(data[3:7])
	m.ColId = binary.BigEndian.Uint32(data[7:11])
	m.MType = binary.BigEndian.Uint16(data[11:13])
	m.Length = data[13]
	m.Bind = data[14]
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
	binary.BigEndian.PutUint32(data[3:7], m.TbId)
	binary.BigEndian.PutUint32(data[7:11], m.ColId)
	binary.BigEndian.PutUint16(data[11:13], m.MType)
	data[13] = m.Length
	data[14] = m.Bind
	data[15] = m.nl
	copy(data[16:], m.data)
	return data
}

func (m *Meta) Clean() {
	m.tp = 0
	m.attr = 0xFF & m.attr
	m.DbId = 0
	m.TbId = 0
	m.ColId = 0
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
