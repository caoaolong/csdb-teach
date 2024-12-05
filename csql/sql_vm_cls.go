package csql

import (
	"csdb-teach/cds"
	"csdb-teach/cfs"
	"csdb-teach/conf"
	"errors"
	"strings"
)

type OpData struct {
	Value  string
	OmCode uint8
}

type SqlVm struct {
	// data map
	dm []OpData
	// code map
	cm map[string]uint8
	// object map
	om map[string]uint8
	// page file map
	pfm map[string]*cfs.PageFile
	// current database name
	cdb *cds.Database
	// databases
	dbs []*cds.Database
}

const (
	_ = iota
	OpCodeCreate
	OpCodeUse
)

const (
	_              = iota
	OmCodeDatabase = 1
	OmCodeTable    = 2
)

func newVm() *SqlVm {
	var vm = new(SqlVm)
	vm.dm = make([]OpData, 0)
	vm.cm = map[string]uint8{
		KwCreate: OpCodeCreate,
		KwUse:    OpCodeUse,
	}
	vm.om = map[string]uint8{
		KwDatabase: OmCodeDatabase,
		KwTable:    OmCodeTable,
	}
	vm.pfm = make(map[string]*cfs.PageFile)
	conf.InitIDFile()
	return vm
}

func NewSqlInc(opcode, object, arg uint8) uint32 {
	var inc uint32 = 0
	inc |= uint32(opcode) << 16
	inc |= uint32(object) << 8
	inc |= uint32(arg)
	return inc
}

func (v *SqlVm) run(instructions []uint32) error {
	for _, instruction := range instructions {
		var opcode = uint8(instruction & 0xFF0000 >> 16)
		var object = uint8(instruction & 0xFF00 >> 8)
		var arg = uint8(instruction & 0xFF)
		err := v.execInstr(opcode, object, arg)
		if err != nil {
			return err
		}
	}
	return nil
}

func (v *SqlVm) execInstr(opcode, object, arg uint8) error {
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
			if err = pf.Flush(); err != nil {
				return err
			}
			v.dm[arg].OmCode = OmCodeDatabase
			v.dbs = append(v.dbs, db)
		}
		return nil
	case OpCodeUse:
		for _, db := range v.dbs {
			if db.Name == v.dm[arg].Value {
				v.cdb = db
				return nil
			}
		}
		return errors.New(conf.ErrDatabaseNotFound)
	}
	return nil
}
