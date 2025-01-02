package row

import (
	"bytes"
	"csdb-teach/conf"
	"encoding/binary"
)

type DataValue struct {
	Attr  uint8
	Value []byte
}

func NewDataValue(attr uint8, data any) (*DataValue, error) {
	var dv = new(DataValue)
	dv.Attr = attr
	if attr&conf.DvNumber == conf.DvNumber {
		dv.Attr |= 8
		dv.Value = make([]byte, 8)
		buf := new(bytes.Buffer)
		err := binary.Write(buf, binary.BigEndian, data.(int64))
		if err != nil {
			return nil, err
		}
		copy(dv.Value, buf.Bytes())
	} else if attr&conf.DvString == conf.DvString {
		var length = uint8(len(data.([]byte)))
		if length <= 31 {
			dv.Attr |= length
			dv.Value = make([]byte, length)
			copy(dv.Value, data.([]byte))
		} else {
			dv.Attr = conf.DvRef | 8
			// TODO: 存储字符串
		}
	} else if attr&conf.DvFloat == conf.DvFloat {
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
