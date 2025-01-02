package cds

import (
	"csdb-teach/cfs"
	"csdb-teach/conf"
	"csdb-teach/row"
)

type Table struct {
	ID   uint32
	Name string
}

func NewTable(pf *cfs.PageFile, db *Database, name string) (*Table, error) {
	var table = new(Table)
	table.Name = name
	tbId, err := conf.IDW.Table()
	if err != nil {
		return nil, err
	}
	table.ID = tbId
	meta, err := row.NewMetaRow(conf.RowTypeTable, 0, db.ID, tbId, 0, 0, table.Name)
	if err != nil {
		return nil, err
	}
	page, err := pf.PageByType(conf.PageTypeMeta, db.ID)
	if err != nil {
		return nil, err
	}
	page.WriteMemory(meta.Encode(), false)
	return table, nil
}
