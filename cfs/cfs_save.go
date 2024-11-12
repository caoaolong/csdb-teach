package cfs

import (
	"fmt"
	"math/rand"
	"os"
	"time"
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

func randomInt(length int) string {
	rand.New(rand.NewSource(time.Now().UnixNano()))

	// 创建一个字符集，包含数字
	const charset = "0123456789"
	result := make([]byte, length)

	for i := range result {
		result[i] = charset[rand.Intn(len(charset))]
	}

	return string(result)
}

func SaveData2(path string, data []byte) error {
	tmp := fmt.Sprintf("%s.tmp.%s", path, randomInt(5))
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
	tmp := fmt.Sprintf("%s.tmp.%s", path, randomInt(5))
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
