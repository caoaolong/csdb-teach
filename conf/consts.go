package conf

const (
	Workspace = "../cs"

	FilePageSize      int = 1024 * 4
	FilePageInitCount int = 1024

	FileHeaderSize = 16
	PageHeaderSize = 16
	RowHeaderSize  = 16

	FileHeaderMagic = "CS.DB"
)

const (
	ErrFileFormat  = "this file is not a page file"
	ErrPageIndex   = "this page index out of range"
	ErrNameTooLong = "this name is too long"
)

const (
	AttrExists    = 0b00000001
	AttrData      = 0b00000100
	AttrStructure = 0b00001000
	AttrString    = 0b00001100
)

const (
	RowTypeDatabase = 0b00000001
	RowTypeTable    = 0b00000010
	RowTypeColumn   = 0b00000100
)
