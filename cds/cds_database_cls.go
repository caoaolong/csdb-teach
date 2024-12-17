package cds

import (
	"csdb-teach/cfs"
	"csdb-teach/conf"
	"csdb-teach/row"
)

type Database struct {
	ID   uint8
	Name string
}

func NewDatabase(pf *cfs.PageFile, name string) (*Database, error) {
	var database = new(Database)
	database.Name = name
	dbId, err := conf.IDW.Database()
	if err != nil {
		return nil, err
	}
	database.ID = dbId
	meta, err := row.NewMetaRow(conf.RowTypeDatabase, 0, dbId, 0, 0, 0, database.Name)
	if err != nil {
		return nil, err
	}
	page, err := pf.PageByType(conf.PageTypeMeta, dbId)
	if err != nil {
		return nil, err
	}
	page.WriteMemory(meta.Encode(), false)
	return database, nil
}
