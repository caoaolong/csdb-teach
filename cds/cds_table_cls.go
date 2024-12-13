package cds

import (
	"csdb-teach/cfs"
	"csdb-teach/conf"
	"csdb-teach/row"
)

type Table struct {
	id   uint32
	name string
}

func NewTable(pf *cfs.PageFile, db *Database, name string) (*Table, error) {
	var table = new(Table)
	table.name = name
	tbId, err := conf.IDW.Table()
	if err != nil {
		return nil, err
	}
	table.id = tbId
	meta, err := row.NewMetaRow(conf.RowTypeTable, 0, db.ID, tbId, 0, 0, table.name)
	if err != nil {
		return nil, err
	}
	page, err := pf.PageByType(conf.PageTypeMeta, db.ID)
	if err != nil {
		return nil, err
	}
	return table, page.Write(pf, meta.Encode(), false)
}
