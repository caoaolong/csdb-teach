package conf

const (
	Workspace = "../cs"

	FilePageSize      int = 1024 * 4
	FilePageInitCount int = 1024
	FileHeaderSize        = 16

	FileHeaderMagic = "CS.DB"
)

const (
	ErrFileFormat = "this file is not a page file"
)
