package row

import (
	"csdb-teach/cfs"
	"csdb-teach/conf"
	"encoding/binary"
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
	data.ColumnCount = colCount
	data.RowID = rowId
	data.Length = uint16(len(values))
	data.data = make([]byte, data.Length)
	copy(data.data, values)
	return data
}

func (d *Data) Read(data []byte) *Data {
	d.DbID = data[0]
	d.TbID = binary.BigEndian.Uint32(data[1:5])
	d.ColumnCount = data[5]
	d.RowID = binary.BigEndian.Uint64(data[6:14])
	d.Length = binary.BigEndian.Uint16(data[14:16])
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
	binary.BigEndian.PutUint64(data[6:14], d.RowID)
	binary.BigEndian.PutUint16(data[14:16], d.Length)
	copy(data[16:], d.data)
	return data
}

func (d *Data) String() string {
	// TODO: 输出行信息
	return ""
}

func (d *Data) Clean() {
	d.DbID = 0
	d.TbID = 0
	d.ColumnCount = 0
	d.RowID = 0
	d.Length = 0
	d.data = nil
}
