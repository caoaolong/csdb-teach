package csql

type Token struct {
	Value  string
	Type   uint8
	OpType uint8
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

var keywords = []string{
	"CREATE", "DATABASE", "TABLE", "VIEW",
}

func NewToken(value string, tType uint8) Token {
	return Token{
		Value: value,
		Type:  tType,
	}
}
