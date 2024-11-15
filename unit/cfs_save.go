package unit

import (
	"csdb-teach/conf"
	"fmt"
	"os"
)

func SaveData1(path string, data []byte) error {
	fp, err := os.OpenFile(path, os.O_WRONLY|os.O_CREATE|os.O_TRUNC, 0664)
	if err != nil {
		return err
	}
	defer func(fp *os.File) {
		err := fp.Close()
		if err != nil {
			os.Exit(1)
		}
	}(fp)
	_, err = fp.Write(data)
	return err
}

func SaveData2(path string, data []byte) error {
	tmp := fmt.Sprintf("%s.tmp.%s", path, conf.RandomInt(5))
	fp, err := os.OpenFile(tmp, os.O_WRONLY|os.O_CREATE|os.O_EXCL, 0664)
	if err != nil {
		return err
	}
	defer func(fp *os.File) {
		err := fp.Close()
		if err != nil {
			os.Exit(1)
		}
	}(fp)
	_, err = fp.Write(data)
	if err != nil {
		err := os.Remove(tmp)
		if err != nil {
			return err
		}
		return err
	}
	return os.Rename(tmp, path)
}

func SaveData3(path string, data []byte) error {
	tmp := fmt.Sprintf("%s.tmp.%s", path, conf.RandomInt(5))
	fp, err := os.OpenFile(tmp, os.O_WRONLY|os.O_CREATE|os.O_EXCL, 0664)
	if err != nil {
		return err
	}
	defer func(fp *os.File) {
		err := fp.Close()
		if err != nil {
			os.Exit(1)
		}
	}(fp)
	_, err = fp.Write(data)
	if err != nil {
		_ = os.Remove(tmp)
		return err
	}
	err = fp.Sync()
	if err != nil {
		_ = os.Remove(tmp)
		return err
	}
	return os.Rename(tmp, path)
}
