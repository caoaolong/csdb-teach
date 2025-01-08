package row

import (
	"csdb-teach/cfs"
	"csdb-teach/conf"
	"encoding/binary"
	"fmt"
)

type Index struct {
	Type    uint8
	Attr    uint8
	TbId    uint32
	ColId   uint32
	Page    uint16
	Offset  uint16
	Unused  uint16
	LPage   uint16
	LOffset uint16
	RPage   uint16
	ROffset uint16
	Value   uint64
}

func NewIndex(page, offset uint16, value uint64) *Index {
	var index = new(Index)

	return index
}

func Left() *Index {
	// TODO: 获取左子节点
	return nil
}

func Right() *Index {
	// TODO: 获取右子节点
	return nil
}

func (i *Index) Read(data []byte) *Index {
	i.Type = data[0]
	i.Attr = data[1]
	i.TbId = binary.BigEndian.Uint32(data[2:6])
	i.ColId = binary.BigEndian.Uint32(data[6:10])
	i.Page = binary.BigEndian.Uint16(data[10:12])
	i.Offset = binary.BigEndian.Uint16(data[12:14])
	i.Unused = binary.BigEndian.Uint16(data[14:16])
	i.LPage = binary.BigEndian.Uint16(data[16:18])
	i.LOffset = binary.BigEndian.Uint16(data[18:20])
	i.RPage = binary.BigEndian.Uint16(data[20:22])
	i.ROffset = binary.BigEndian.Uint16(data[22:24])
	i.Value = binary.BigEndian.Uint64(data[24:32])
	return nil
}

func (i *Index) Encode() []byte {
	var data = make([]byte, conf.IndexRowSize)
	data[0] = i.Type
	data[1] = i.Attr
	binary.BigEndian.PutUint32(data[2:6], i.TbId)
	binary.BigEndian.PutUint32(data[6:10], i.ColId)
	binary.BigEndian.PutUint16(data[10:12], i.Page)
	binary.BigEndian.PutUint16(data[12:14], i.Offset)
	binary.BigEndian.PutUint16(data[14:16], i.Unused)
	binary.BigEndian.PutUint16(data[16:18], i.LPage)
	binary.BigEndian.PutUint16(data[18:20], i.LOffset)
	binary.BigEndian.PutUint16(data[20:22], i.RPage)
	binary.BigEndian.PutUint16(data[22:24], i.ROffset)
	binary.BigEndian.PutUint64(data[24:32], i.Value)
	return nil
}

func (i *Index) Decode(page *cfs.Page, offset int64) *Index {
	i.Read(page.Raw()[offset:])
	return i
}

func (i *Index) String() string {
	return fmt.Sprintf("Current: (%d,%d)=%d, Left: (%d,%d), Right: (%d,%d)\n",
		i.Page, i.Offset, i.Value, i.LPage, i.LOffset, i.RPage, i.ROffset)
}

func (i *Index) Clean() {
	i.Type = 0
	i.Attr = 0
}
