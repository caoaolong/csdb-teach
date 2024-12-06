package utils

import (
	"csdb-teach/cds"
	"csdb-teach/row"
)

func ToDatabase(m *row.Meta) *cds.Database {
	var db = new(cds.Database)
	db.Id = m.DbId
	db.Name = m.String()
	return db
}
