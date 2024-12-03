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
	ErrFileFormat   = "this file is not a page file"
	ErrPageIndex    = "this page index out of range"
	ErrNameTooLong  = "this name is too long"
	ErrRowType      = "this is an unknown row type"
	ErrPageNotFound = "can't find the page of the specified type"
	ErrSyntax       = "syntax error"
)

const (
	AttrExists    = 0b00000001
	AttrData      = 0b00000100
	AttrStructure = 0b00001000
	AttrString    = 0b00001100

	PageTypeMeta   = 0b00001000
	PageTypeData   = 0b00010000
	PageTypeString = 0b00100000
	PageTypeIndex  = 0b00011000
	PageTypeMask   = 0b00111000
)

const (
	RowTypeDatabase = 0b00000001
	RowTypeTable    = 0b00000010
	RowTypeColumn   = 0b00000100
	RowTypeNull     = 0b11111111
	RowTypeUnknown  = 0b00000000
)

const (
	ColumnTypeTinyInt    = 0b0000_0000_0000_0001
	ColumnTypeMediumInt  = 0b0000_0000_0000_0010
	ColumnTypeDefaultInt = 0b0000_0000_0000_0100
	ColumnTypeBigIntInt  = 0b0000_0000_0000_1000
	ColumnTypeBit        = 0b0000_0000_0000_0011

	ColumnTypeFloat  = 0b0000_0000_0001_0000
	ColumnTypeDouble = 0b0000_0000_0010_0000

	ColumnTypeDate     = 0b0000_0000_0100_0000
	ColumnTypeTime     = 0b0000_0000_1000_0000
	ColumnTypeDateTime = 0b0000_0000_1100_0000

	ColumnTypeVarchar    = 0b0000_0001_0000_0000
	ColumnTypeNchar      = 0b0000_0011_0000_0000
	ColumnTypeTinytext   = 0b0000_0010_0000_0000
	ColumnTypeText       = 0b0000_0100_0000_0000
	ColumnTypeMediumText = 0b0000_1000_0000_0000
	ColumnTypeBlob       = 0b0000_1111_0000_0000
)
