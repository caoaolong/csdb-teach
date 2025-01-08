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

func ToTable(m *row.Meta) *cds.Table {
	var tb = new(cds.Table)
	tb.ID = m.TbId
	tb.Name = m.String()
	return tb
}

func ToColumn(m *row.Meta) *cds.Column {
	var col = new(cds.Column)
	col.ID = m.ColId
	col.Name = m.String()
	col.Meta = m
	return col
}
