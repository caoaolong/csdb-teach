package row

import (
	"csdb-teach/cfs"
	"csdb-teach/conf"
	"encoding/binary"
	"fmt"
	"strings"
)

type Data struct {
	DbID        uint8
	TbID        uint32
	ColumnCount uint8
	RowID       uint64
	Length      uint16
	data        []byte
}

func NewEmptyData() *Data {
	var data = new(Data)
	return data
}

func NewDataRow(dbId uint8, tbId uint32, colCount uint8, rowId uint64, values []byte) *Data {
	var data = new(Data)
	data.DbID = dbId
	data.TbID = tbId
	data.RowID = rowId
	if values != nil {
		data.SetValue(values, colCount)
	}
	return data
}

func (d *Data) SetValue(values []byte, colCount uint8) {
	d.ColumnCount = colCount
	if d.data == nil {
		d.Length = uint16(len(values))
		d.data = make([]byte, d.Length)
	}
	copy(d.data, values)
}

func (d *Data) Read(data []byte) *Data {
	d.DbID = data[0]
	d.TbID = binary.BigEndian.Uint32(data[1:5])
	d.ColumnCount = data[5]
	d.RowID = binary.BigEndian.Uint64(data[6:14])
	d.Length = binary.BigEndian.Uint16(data[14:16])
	d.data = make([]byte, d.Length)
	copy(d.data, data[16:16+d.Length])
	return nil
}

func (d *Data) Decode(page *cfs.Page, offset int64) *Data {
	d.Read(page.Raw()[offset:])
	return d
}

func (d *Data) Encode() []byte {
	var data = make([]byte, conf.RowHeaderSize+d.Length)
	data[0] = d.DbID
	binary.BigEndian.PutUint32(data[1:5], d.TbID)
	data[5] = d.ColumnCount
	binary.BigEndian.PutUint64(data[6:14], d.RowID)
	binary.BigEndian.PutUint16(data[14:16], d.Length)
	copy(data[16:], d.data)
	return data
}

func (d *Data) String() string {
	var value strings.Builder
	var data = d.data
	var idx = 0
	for i := 0; i < int(d.ColumnCount); i++ {
		var attr = data[idx]
		var cl = attr & conf.LenMask
		if attr&conf.DvNumber == conf.DvNumber {
			value.WriteString(fmt.Sprintf("%d", binary.BigEndian.Uint64(data[idx+1:idx+1+int(cl)])))
		} else if attr&conf.DvString == conf.DvString {
			value.Write(data[idx+1 : idx+1+int(cl)])
		}
		idx += int(cl) + 1
		if i < int(d.ColumnCount-1) {
			value.WriteByte(',')
		}
	}
	return value.String()
}

func (d *Data) Clean() {
	d.DbID = 0
	d.TbID = 0
	d.ColumnCount = 0
	d.RowID = 0
	d.Length = 0
	d.data = nil
}
