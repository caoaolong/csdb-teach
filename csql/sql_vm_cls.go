package csql

import (
	"csdb-teach/cds"
	"csdb-teach/cfs"
	"csdb-teach/conf"
	"strings"
)

type SqlVm struct {
	dm  []string
	cm  map[string]uint8
	om  map[string]uint8
	pfm map[string]*cfs.PageFile
}

const (
	OpCodeCreate = 1

	OmCodeDatabase = 1
	OmCodeTable    = 2
)

func newVm() *SqlVm {
	var vm = new(SqlVm)
	vm.dm = make([]string, 0)
	vm.cm = map[string]uint8{
		"CREATE": OpCodeCreate,
	}
	vm.om = map[string]uint8{
		"DATABASE": OmCodeDatabase,
		"TABLE":    OmCodeTable,
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
			var name = v.dm[arg]
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
			_, err = cds.NewDatabase(pf, name)
			if err != nil {
				return err
			}
		}
	}
	return nil
}
