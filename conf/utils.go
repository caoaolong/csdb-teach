package conf

import (
	"math/rand"
	"strconv"
	"time"
)

func RandomInt(length int) string {
	rand.New(rand.NewSource(time.Now().UnixNano()))

	// 创建一个字符集，包含数字
	const charset = "0123456789"
	result := make([]byte, length)

	for i := range result {
		result[i] = charset[rand.Intn(len(charset))]
	}

	return string(result)
}

func RowType(tp byte) uint8 {
	if RowTypeColumn&tp == RowTypeColumn {
		return RowTypeColumn
	} else if RowTypeTable&tp == RowTypeTable {
		return RowTypeTable
	} else if RowTypeDatabase&tp == RowTypeDatabase {
		return RowTypeDatabase
	} else if RowTypeNull&tp == RowTypeNull {
		return RowTypeNull
	} else {
		return RowTypeUnknown
	}
}

func IsNumber(v string) (bool, int64) {
	n, err := strconv.ParseInt(v, 10, 64)
	return err == nil, n
}
