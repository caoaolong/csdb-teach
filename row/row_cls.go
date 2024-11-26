package row

import "csdb-teach/cfs"

type Row[T any] interface {
	Encode() []byte
	Decode(page *cfs.Page, offset int64) T
	String() string
	Clean()
}
