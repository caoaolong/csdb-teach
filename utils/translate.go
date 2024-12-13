package utils

import (
	"csdb-teach/cds"
	"csdb-teach/row"
)

func ToDatabase(m *row.Meta) *cds.Database {
	var db = new(cds.Database)
	db.ID = m.DbId
	db.Name = m.String()
	return db
}

func ToColumn(m *row.Meta) *cds.Column {
	var col = new(cds.Column)
	col.ID = m.ColId
	col.Name = m.String()
	return col
}
