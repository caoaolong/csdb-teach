package row

import (
	"csdb-teach/conf"
	"encoding/binary"
	"errors"
	"math"
	"strings"
)

type Meta struct {
	tp     uint8
	attr   uint8
	dbId   uint8
	tbId   uint32
	colId  uint32
	unused uint32
	nl     uint8
	data   []byte
}

func NewMetaRow(tp, attr, dbId uint8, tbId, colId uint32, value string) (*Meta, error) {
	var nl = len(value)
	if nl > math.MaxUint8 {
		return nil, errors.New(conf.ErrNameTooLong)
	}
	var meta = new(Meta)
	meta.tp = tp
	meta.attr = attr
	meta.dbId = dbId
	meta.tbId = tbId
	meta.colId = colId
	meta.unused = 0
	meta.nl = uint8(len(value))
	meta.data = make([]byte, meta.nl)
	copy(meta.data, strings.ToUpper(value))
	return meta, nil
}

func (m *Meta) Encode() []byte {
	var data = make([]byte, conf.RowHeaderSize+m.nl)
	data[0] = m.tp
	data[1] = m.attr
	data[2] = m.dbId
	binary.BigEndian.PutUint32(data[3:7], m.tbId)
	binary.BigEndian.PutUint32(data[7:11], m.tbId)
	binary.BigEndian.PutUint32(data[11:15], m.unused)
	data[15] = m.nl
	copy(data[16:], m.data)
	return data
}

func (m *Meta) DecodeMeta(data []byte) (*Meta, error) {
	var meta = new(Meta)
	m.tp = data[0]
	m.attr = data[1]
	m.dbId = data[2]
	m.tbId = binary.BigEndian.Uint32(data[3:7])
	m.tbId = binary.BigEndian.Uint32(data[7:11])
	m.unused = binary.BigEndian.Uint32(data[11:15])
	m.nl = data[15]
	m.data = make([]byte, m.nl)
	copy(meta.data, data[16:])
	return meta, nil
}
