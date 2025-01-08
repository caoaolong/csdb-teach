package csql

import (
	"csdb-teach/cds"
	"csdb-teach/cfs"
	"csdb-teach/conf"
	"csdb-teach/row"
	"csdb-teach/utils"
	"strings"
	"unsafe"
)

type OpData struct {
	Value  string
	OmCode uint8
}

type SqlVm struct {
	/********* memory **********/
	// data map
	dm []OpData
	// code map
	cm map[string]uint8
	// object map
	om map[string]uint8
	// datatype map
	dtm map[string]uint16
	// page file map
	pfm map[string]*cfs.PageFile

	/********* registers **********/
	// database pointer register
	dpr int64
	// table pointer register
	tpr int64
	// column pointer register
	cpr int64
	// page file register
	pfr int64
	// row pointer register
	rpr int64

	/********* attributes **********/
	// databases
	dbs []*cds.Database
	// values
	values []*row.DataValue
}

const (
	_ = iota
	OpCodeCreate
	OpCodeUse
	OpCodeSet
	OpCodeInsert
	OpCodeInsertBegin
	OpCodeInsertEnd
	OpCodeInto
)

const (
	_ = iota
	OmCodeDatabase
	OmCodeTable
	OmCodeColumn
)

func newVm() *SqlVm {
	var vm = new(SqlVm)
	vm.dm = make([]OpData, 0)
	vm.values = make([]*row.DataValue, 0)
	vm.cm = map[string]uint8{
		KwCreate: OpCodeCreate,
		KwInsert: OpCodeInsert,
		KwInto:   OpCodeInto,
		KwUse:    OpCodeUse,
	}
	vm.om = map[string]uint8{
		KwDatabase: OmCodeDatabase,
		KwTable:    OmCodeTable,
	}
	vm.dtm = map[string]uint16{
		DtInt:      conf.ColumnTypeDefaultInt,
		DtBigInt:   conf.ColumnTypeBigInt,
		DtSmallInt: conf.ColumnTypeSmallInt,
		DtDouble:   conf.ColumnTypeDouble,
		DtFloat:    conf.ColumnTypeFloat,
		DtVarChar:  conf.ColumnTypeVarchar,
	}
	vm.pfm = make(map[string]*cfs.PageFile)
	conf.InitIDFile()
	return vm
}

func NewSqlIncWithVal(opcode, object, arg uint8, attr uint16, val uint8) uint64 {
	var inc = NewSqlInc(opcode, object, arg, attr)
	inc |= uint64(val) << 40
	return inc
}

func NewSqlInc(opcode, object, arg uint8, attr uint16) uint64 {
	var inc uint64 = 0
	inc |= uint64(attr) << 24
	inc |= uint64(opcode) << 16
	inc |= uint64(object) << 8
	inc |= uint64(arg)
	return inc
}

func (v *SqlVm) run(instructions []uint64) error {
	for _, instruction := range instructions {
		var data = uint8(instruction & 0xFF0000000000 >> 40)
		var attr = uint16(instruction & 0xFFFF000000 >> 24)
		var opcode = uint8(instruction & 0xFF0000 >> 16)
		var object = uint8(instruction & 0xFF00 >> 8)
		var arg = uint8(instruction & 0xFF)
		err := v.execInstr(opcode, object, arg, attr, data)
		if err != nil {
			return err
		}
	}
	for _, pf := range v.pfm {
		if err := pf.Flush(); err != nil {
			return err
		}
	}
	return nil
}

func (v *SqlVm) execInstr(opcode, object, arg uint8, attr uint16, d uint8) error {
	switch opcode {
	case OpCodeCreate:
		switch object {
		case OmCodeDatabase:
			var name = v.dm[arg].Value
			var pf = v.pfm[name]
			if pf == nil {
				v.pfm[name] = new(cfs.PageFile)
				pf = v.pfm[name]
			}
			var dbName = strings.ToLower(name)
			err := pf.Open(dbName)
			if err != nil {
				return err
			}
			var db *cds.Database
			if db, err = cds.NewDatabase(pf, name); err != nil {
				return err
			}
			v.dm[arg].OmCode = OmCodeDatabase
			v.dbs = append(v.dbs, db)
			break
		case OmCodeTable:
			var pf = (*cfs.PageFile)(unsafe.Pointer(uintptr(v.pfr)))
			var db = (*cds.Database)(unsafe.Pointer(uintptr(v.dpr)))
			var name = v.dm[arg].Value
			tb, err := cds.NewTable(pf, db, name)
			if err != nil {
				return err
			}
			v.tpr = int64(uintptr(unsafe.Pointer(tb)))
			break
		case OmCodeColumn:
			var pf = (*cfs.PageFile)(unsafe.Pointer(uintptr(v.pfr)))
			var db = (*cds.Database)(unsafe.Pointer(uintptr(v.dpr)))
			var tb = (*cds.Table)(unsafe.Pointer(uintptr(v.tpr)))
			var name = v.dm[arg].Value
			col, err := cds.NewColumn(pf, db, tb, name, attr)
			if err != nil {
				return err
			}
			v.cpr = int64(uintptr(unsafe.Pointer(col)))
			break
		}
		break
	case OpCodeUse:
		// 查找当前已经打开的数据库中是否存在该数据库
		var dbName = strings.ToLower(v.dm[arg].Value)
		for _, db := range v.dbs {
			if db.Name == v.dm[arg].Value {
				v.dpr = int64(uintptr(unsafe.Pointer(db)))
				v.pfr = int64(uintptr(unsafe.Pointer(v.pfm[dbName])))
				return nil
			}
		}
		// 如果不存在则尝试从磁盘中读取
		v.pfm[dbName] = new(cfs.PageFile)
		err := v.pfm[dbName].Read(dbName)
		if err != nil {
			return err
		}
		page, err := v.pfm[dbName].Page(0, true)
		if err != nil {
			return err
		}
		data, err := page.FindRowByName(conf.RowTypeDatabase, dbName)
		if err != nil {
			return err
		}
		var db = utils.ToDatabase(row.NewEmptyMeta().Read(data))
		v.dbs = append(v.dbs, db)
		v.dpr = int64(uintptr(unsafe.Pointer(db)))
		v.pfr = int64(uintptr(unsafe.Pointer(v.pfm[dbName])))
		break
	case OpCodeSet:
		switch object {
		case OmCodeColumn:
			var col = (*cds.Column)(unsafe.Pointer(uintptr(v.cpr)))
			if arg == conf.SetTypeLength {
				err := col.SetLength(uint8(attr))
				if err != nil {
					return err
				}
			} else if arg == conf.SetTypeBind {
				err := col.SetBind(uint8(attr))
				if err != nil {
					return err
				}
			}
			break
		}
		break
	case OpCodeInsertBegin:
		var db = (*cds.Database)(unsafe.Pointer(uintptr(v.dpr)))
		var tb = (*cds.Table)(unsafe.Pointer(uintptr(v.tpr)))
		if tb == nil {
			page, err := v.pfm[strings.ToLower(db.Name)].Page(0, true)
			if err != nil {
				return err
			}
			data, err := page.FindRowByName(conf.RowTypeTable, v.dm[arg].Value)
			if err != nil {
				return err
			}
			tb = utils.ToTable(row.NewEmptyMeta().Read(data))
			v.tpr = int64(uintptr(unsafe.Pointer(tb)))
		}
		rowId, err := conf.IDW.Value()
		if err != nil {
			return err
		}
		r := row.NewDataRow(db.ID, tb.ID, 0, rowId, nil)
		v.rpr = int64(uintptr(unsafe.Pointer(r)))
		break
	case OpCodeInsertEnd:
		var pf = (*cfs.PageFile)(unsafe.Pointer(uintptr(v.pfr)))
		var db = (*cds.Database)(unsafe.Pointer(uintptr(v.dpr)))
		var r = (*row.Data)(unsafe.Pointer(uintptr(v.rpr)))
		var data = make([]byte, 0)
		for _, e := range v.values {
			data = append(data, row.DataValueBytes(e)...)
		}
		r.SetValue(data, uint8(len(v.values)))
		v.values = make([]*row.DataValue, 0)
		page, err := pf.PageByType(conf.PageTypeData, db.ID)
		if err != nil {
			return err
		}
		page.WriteMemory(r.Encode(), false)
		break
	case OpCodeInsert:
		switch object {
		case OmCodeColumn:
			var db = (*cds.Database)(unsafe.Pointer(uintptr(v.dpr)))
			page, err := v.pfm[strings.ToLower(db.Name)].Page(0, true)
			if err != nil {
				return err
			}
			data, err := page.FindRowByName(conf.RowTypeColumn, v.dm[arg].Value)
			if err != nil {
				return err
			}
			var column = utils.ToColumn(row.NewEmptyMeta().Read(data))
			if column.IsPrimaryKey() {
				// TODO: 处理主键
			}
			var dv any
			if attr&conf.DvNumber == conf.DvNumber {
				dv = int64(d)
			} else if attr&conf.DvString == conf.DvString {
				dv = []byte(v.dm[d].Value)
			}
			rv, err := row.NewDataValue(uint8(attr), dv)
			if err != nil {
				return err
			}
			v.values = append(v.values, rv)
		}
	}
	return nil
}
