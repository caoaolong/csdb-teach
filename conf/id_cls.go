package conf

import (
	"encoding/binary"
	"io"
	"log"
	"os"
)

const idFilename = Workspace + "/db.ids"

type IDWare struct {
	database uint8
	table    uint32
	column   uint64
	value    uint64
}

var IDW = IDWare{}

var fp *os.File = nil

func (i *IDWare) Database() (uint8, error) {
	var v = i.database
	i.database++
	return v, flushIDWare()
}

func (i *IDWare) Table() (uint32, error) {
	var v = i.table
	i.table++
	return v, flushIDWare()
}

func (i *IDWare) Column() (uint64, error) {
	var v = i.column
	i.column++
	return v, flushIDWare()
}

func (i *IDWare) Value() (uint64, error) {
	var v = i.value
	i.value++
	return v, flushIDWare()
}

func InitIDFile() {
	var err error
	fp, err = os.OpenFile(idFilename, os.O_RDWR, 0644)
	if err != nil {
		fp, err = os.OpenFile(idFilename, os.O_RDWR|os.O_CREATE, 0644)
		err = createIDWare()
		if err != nil {
			log.Fatal(err)
		}
		return
	}
	data, err := io.ReadAll(fp)
	if err != nil {
		log.Fatal(err)
	}
	if err = readIDWare(data); err != nil {
		log.Fatal(err)
	}
}

func readIDWare(data []byte) error {
	IDW.database = data[0]
	IDW.table = binary.BigEndian.Uint32(data[1:5])
	IDW.column = binary.BigEndian.Uint64(data[5:13])
	IDW.value = binary.BigEndian.Uint64(data[13:21])
	return nil
}

func createIDWare() error {
	var data = make([]byte, 21)
	data[0] = 1
	IDW.database = 1
	binary.BigEndian.PutUint32(data[1:5], 1)
	IDW.table = 1
	binary.BigEndian.PutUint64(data[5:13], 1)
	IDW.column = 1
	binary.BigEndian.PutUint64(data[13:21], 1)
	IDW.value = 1
	_, err := fp.Write(data)
	if err != nil {
		return err
	}
	return fp.Sync()
}

func flushIDWare() error {
	var data = make([]byte, 21)
	data[0] = IDW.database
	binary.BigEndian.PutUint32(data[1:5], IDW.table)
	binary.BigEndian.PutUint64(data[5:13], IDW.column)
	binary.BigEndian.PutUint64(data[13:21], IDW.value)
	_, err := fp.Seek(0, io.SeekStart)
	if err != nil {
		return err
	}
	_, err = fp.Write(data)
	if err != nil {
		return err
	}
	return fp.Sync()
}

func CloseIDWare() error {
	return fp.Close()
}
