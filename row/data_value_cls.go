package row

import (
	"bytes"
	"encoding/binary"
)

type DataValue struct {
	Attr  uint8
	Value []byte
}

const (
	DvNumber = 0b00100000
	DvString = 0b01000000
	DvRef    = 0b10000000
	DvFloat  = 0b01100000
)

func NewDataValue(attr uint8, data any) (*DataValue, error) {
	var dv = new(DataValue)
	dv.Attr = attr
	if attr&DvNumber == DvNumber {
		dv.Attr |= 8
		dv.Value = make([]byte, 8)
		buf := new(bytes.Buffer)
		err := binary.Write(buf, binary.BigEndian, data.(int64))
		if err != nil {
			return nil, err
		}
		copy(dv.Value, buf.Bytes())
	} else if attr&DvString == DvString {
		var length = uint8(len(data.([]byte)))
		if length <= 31 {
			dv.Attr |= length
			dv.Value = make([]byte, length)
			copy(dv.Value, data.([]byte))
		} else {
			dv.Attr = DvRef | 8
			// TODO: 存储字符串
		}
	} else if attr&DvFloat == DvFloat {
		dv.Attr |= 8
		dv.Value = make([]byte, 8)
		buf := new(bytes.Buffer)
		err := binary.Write(buf, binary.BigEndian, data.(float64))
		if err != nil {
			return nil, err
		}
		copy(dv.Value, buf.Bytes())
	}
	return dv, nil
}

func DataValueBytes(dv *DataValue) []byte {
	var data = make([]byte, dv.Attr&0b11111+1)
	data[0] = dv.Attr
	copy(data[1:], dv.Value)
	return data
}
