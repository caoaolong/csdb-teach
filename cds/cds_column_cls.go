package cds

import (
	"csdb-teach/cfs"
	"csdb-teach/conf"
	"csdb-teach/row"
)

type Column struct {
	ID   uint32
	Attr uint8
	Name string

	pf   *cfs.PageFile
	page *cfs.Page
	meta *row.Meta
}

func NewColumn(pf *cfs.PageFile, db *Database, tb *Table, name string, mType uint16) (*Column, error) {
	var column = new(Column)
	column.Name = name
	column.pf = pf
	colId, err := conf.IDW.Column()
	if err != nil {
		return nil, err
	}
	column.ID = uint32(colId)
	meta, err := row.NewMetaRow(conf.RowTypeColumn, column.Attr, db.ID, tb.id, uint32(colId), mType, column.Name)
	if err != nil {
		return nil, err
	}
	column.meta = meta
	page, err := pf.PageByType(conf.PageTypeMeta, db.ID)
	if err != nil {
		return nil, err
	}
	column.page = page
	page.WriteMemory(meta.Encode(), false)
	return column, nil
}

func (c *Column) SetLength(value uint8) error {
	data, offset, err := c.page.FindRowByID(conf.RowTypeColumn, c.ID)
	if err != nil {
		return err
	}
	c.meta.Read(data).Length = value
	c.page.Cover(offset, c.meta.Encode())
	return nil
}

func (c *Column) SetBind(value uint8) error {
	data, offset, err := c.page.FindRowByID(conf.RowTypeColumn, c.ID)
	if err != nil {
		return err
	}
	c.meta.Read(data).Bind = value
	c.page.Cover(offset, c.meta.Encode())
	return nil
}
