package cds

import (
	"csdb-teach/cfs"
	"csdb-teach/conf"
	"csdb-teach/row"
)

type Column struct {
	id    uint32
	cType uint8
	name  string
}

func NewColumn(pf *cfs.PageFile, db *Database, tb *Table, name string, mType uint16) (*Column, error) {
	var column = new(Column)
	column.name = name
	colId, err := conf.IDW.Column()
	if err != nil {
		return nil, err
	}
	column.id = uint32(colId)
	meta, err := row.NewMetaRow(conf.RowTypeColumn, column.cType, db.Id, tb.id, uint32(colId), mType, column.name)
	if err != nil {
		return nil, err
	}
	page, err := pf.PageByType(conf.PageTypeMeta, db.Id)
	if err != nil {
		return nil, err
	}
	return column, page.Write(pf, meta.Encode(), false)
}
