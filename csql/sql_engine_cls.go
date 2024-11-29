package csql

import (
	list "github.com/duke-git/lancet/v2/datastructure/list"
	"slices"
	"strings"
	"sync"
)

type SqlEngine struct {
	tokens  list.List[Token]
	entries [][]*Token
}

var _se *SqlEngine
var _seOnce sync.Once

// CM code map
var CM = map[string]uint8{
	"CREATE": 1,
}

// OM object map
var OM = map[string]uint8{
	"DATABASE": 1,
	"TABLE":    2,
}

func NewSqlEngine() *SqlEngine {
	_seOnce.Do(func() {
		_se = new(SqlEngine)
	})
	return _se
}

func (s *SqlEngine) PushToken(token Token) {
	if token.Value == "" {
		return
	}
	var value = strings.ToUpper(token.Value)
	if slices.Contains(keywords, value) {
		token.Type = TokenTypeKeyword
		if CM[value] > 0 {
			token.OpType = OpTypeCode
		} else if OM[value] > 0 {
			token.OpType = OpTypeObject
		}
	} else {
		token.OpType = OpTypeData
	}
	s.tokens.Push(token)
}

func (s *SqlEngine) Tokens() []Token {
	return s.tokens.Data()
}

func (s *SqlEngine) Entries() [][]*Token {
	return s.entries
}

func (s *SqlEngine) ParseToken(script string) {
	var value strings.Builder
	for _, char := range script {
		switch char {
		case ' ', '\t', '\n', '\r':
			s.PushToken(NewToken(value.String(), TokenTypeIdentifier))
			value.Reset()
			break
		case ';':
			s.PushToken(NewToken(value.String(), TokenTypeIdentifier))
			value.Reset()
			s.PushToken(NewToken(";", TokenTypeDelimiter))
		case ',', '(', ')':
			s.PushToken(NewToken(value.String(), TokenTypeSymbol))
			break
		default:
			value.WriteRune(char)
		}
	}
	s.PushToken(NewToken(value.String(), TokenTypeIdentifier))
}

func (s *SqlEngine) ParseSyntax() {
	s.entries = make([][]*Token, 0)
	var entry = make([]*Token, 0)
	for _, token := range s.Tokens() {
		if token.Type == TokenTypeDelimiter {
			s.entries = append(s.entries, entry)
			entry = make([]*Token, 0)
		} else {
			entry = append(entry, &token)
		}
	}
}

func (s *SqlEngine) Compile() {
	// TODO: 构建指令

	// TODO: 启动虚拟机执行
}
