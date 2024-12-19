package csql

type Token struct {
	Value   string
	Type    uint8
	OpType  uint8
	OpValue uint16
	OpBind  uint8
}

const (
	TokenTypeUnknown = iota
	TokenTypeKeyword
	TokenTypeIdentifier
	TokenTypeNumber
	TokenTypeString
	TokenTypeSymbol
	TokenTypeDelimiter
	TokenTypeDataType
	TokenTypeComment
)

const (
	_          int = iota
	OpTypeCode     = iota
	OpTypeObject
	OpTypeData
	OpTypeAttr
	OpTypeBind
)

const (
	KwCreate   = "CREATE"
	KwInsert   = "INSERT"
	KwInto     = "INTO"
	KwValues   = "VALUES"
	KwDatabase = "DATABASE"
	KwTable    = "TABLE"
	KwView     = "VIEW"
	KwUse      = "USE"
)

var keywords = []string{
	KwCreate, KwInsert, KwInto, KwValues,
	KwDatabase, KwTable, KwView, KwUse,
}

const (
	DtInt      = "INT"
	DtTinyInt  = "TINYINT"
	DtSmallInt = "SMALLINT"
	DtBigInt   = "BIGINT"
	DtFloat    = "FLOAT"
	DtDouble   = "DOUBLE"
	DtVarChar  = "VARCHAR"
)

var datatypes = []string{
	DtInt,
	DtTinyInt,
	DtSmallInt,
	DtBigInt,
	DtFloat,
	DtDouble,
}

const (
	ctNot     = "NOT"
	ctNull    = "NULL"
	ctUnique  = "UNIQUE"
	ctPrimary = "PRIMARY"
	ctKey     = "KEY"
	ctDefault = "DEFAULT"
	ctComment = "COMMENT"
)

var constraints = []string{
	ctNot, ctNull, ctUnique, ctPrimary, ctKey, ctDefault, ctComment,
}

func NewToken(value string, tType uint8) Token {
	return Token{
		Value: value,
		Type:  tType,
	}
}
