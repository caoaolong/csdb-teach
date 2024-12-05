package csql

type Token struct {
	Value   string
	Type    uint8
	OpType  uint8
	OpValue uint8
}

const (
	TokenTypeUnknown = iota
	TokenTypeKeyword
	TokenTypeIdentifier
	TokenTypeNumber
	TokenTypeString
	TokenTypeSymbol
	TokenTypeDelimiter
	TokenTypeComment
)

const (
	_          int = iota
	OpTypeCode     = iota
	OpTypeObject
	OpTypeData
	OpTypeAttr
)

const (
	KwCreate   = "CREATE"
	KwDatabase = "DATABASE"
	KwTable    = "TABLE"
	KwView     = "VIEW"
	KwUse      = "USE"
)

var keywords = []string{
	KwCreate, KwDatabase, KwTable, KwView, KwUse,
}

func NewToken(value string, tType uint8) Token {
	return Token{
		Value: value,
		Type:  tType,
	}
}
