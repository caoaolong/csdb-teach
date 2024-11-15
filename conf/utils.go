package conf

import (
	"math/rand"
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
