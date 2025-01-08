package conf

const (
	Workspace = "../cs"

	FsTypeNtfs  = "NTFS"
	FsTypeFat32 = "FAT32"

	FilePageSize         int = 1024 * 4
	FilePageInitCount    int = 1024
	FilePageNtfsMaxCount int = 4 * 1024 * 1024 * 1024 * 1024 * 1024 // NTFS单个文件最大为16EB
	FilePageFat32MxCount     = 1 * 1024 * 1024                      // FAT32单个文件最大为4GB

	FileHeaderSize = 16
	PageHeaderSize = 16
	RowHeaderSize  = 16
	IndexRowSize   = RowHeaderSize * 2

	FileHeaderMagic = "CS.DB"
)

var FsMaxPageCount = map[string]int{
	FsTypeNtfs:  FilePageNtfsMaxCount,
	FsTypeFat32: FilePageFat32MxCount,
}

const (
	ErrFileFormat       = "this file is not a page file"
	ErrPageIndex        = "this page index out of range"
	ErrNameTooLong      = "this name is too long"
	ErrRowType          = "this is an unknown row type"
	ErrPageNotFound     = "can't find the page of the specified type"
	ErrSyntax           = "syntax error"
	ErrPageFileFull     = "page file is full"
	ErrDatabaseNotFound = "can't find the database"
)

const (
	AttrExists    = 0b00000001
	AttrData      = 0b00000100
	AttrStructure = 0b00001000
	AttrString    = 0b00001100

	SetTypeLength = 1
	SetTypeBind   = 2

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
	ColumnTypeSmallInt   = 0b0000_0000_0000_0010
	ColumnTypeDefaultInt = 0b0000_0000_0000_0100
	ColumnTypeBigInt     = 0b0000_0000_0000_1000
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

const (
	FieldNotNull    = 0b00000001
	FieldPrimaryKey = 0b00000010
	FieldDefault    = 0b00000100
	FieldUnique     = 0b00001000
	FieldComment    = 0b00010000
)

const (
	DvNumber = 0b00100000
	DvString = 0b01000000
	DvRef    = 0b10000000
	DvFloat  = 0b01100000
	LenMask  = 0b00011111
)
