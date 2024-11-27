package cds

import (
	"csdb-teach/cfs"
	"csdb-teach/conf"
	"csdb-teach/row"
)

type Database struct {
	id   uint8
	name string
}

func NewDatabase(pf *cfs.PageFile, name string) (*Database, error) {
	var database = new(Database)
	database.name = name
	dbId, err := conf.IDW.Database()
	if err != nil {
		return nil, err
	}
	database.id = dbId
	meta, err := row.NewMetaRow(conf.RowTypeDatabase, 0, dbId, 0, 0, 0, database.name)
	if err != nil {
		return nil, err
	}
	page, err := pf.PageByType(conf.PageTypeMeta, dbId)
	if err != nil {
		return nil, err
	}
	return database, page.Write(pf, meta.Encode(), false)
}
