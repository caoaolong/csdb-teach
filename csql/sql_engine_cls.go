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
	vm      *SqlVm
}

var _se *SqlEngine
var _seOnce sync.Once

func NewSqlEngine() *SqlEngine {
	_seOnce.Do(func() {
		_se = new(SqlEngine)
		_se.vm = newVm()
	})
	return _se
}

func (s *SqlEngine) Run(instructions []uint32) error {
	return s.vm.run(instructions)
}

func (s *SqlEngine) Close() {
	for _, v := range s.vm.pfm {
		_ = v.Close()
	}
}

func (s *SqlEngine) PushData(value string) uint8 {
	var index = len(s.vm.dm)
	s.vm.dm = append(s.vm.dm, value)
	return uint8(index)
}

func (s *SqlEngine) PushToken(token Token) {
	if token.Value == "" {
		return
	}
	var value = strings.ToUpper(token.Value)
	var opType uint8 = 0
	var opValue uint8 = 0
	if slices.Contains(keywords, value) {
		opValue = s.vm.cm[value]
		opType = OpTypeCode
		if opValue == 0 {
			opValue = s.vm.om[value]
			opType = OpTypeObject
			if opValue == 0 {
				opType = OpTypeData
				opValue = s.PushData(token.Value)
			}
		}
	} else {
		opType = OpTypeData
		opValue = s.PushData(token.Value)
	}
	token.OpType = opType
	token.OpValue = opValue
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

func (s *SqlEngine) ParseSyntax() ([]*ASTTree, error) {
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
	var trees = make([]*ASTTree, 0)
	for _, e := range s.entries {
		tree, err := NewASTTree(s, e).Build()
		if err != nil {
			return trees, err
		}
		trees = append(trees, tree)
	}
	return trees, nil
}

func (s *SqlEngine) Compile(trees []*ASTTree) ([]uint32, error) {
	var instructions = make([]uint32, 0)
	for _, tree := range trees {
		switch tree.Root.OpValue {
		case OpCodeCreate:
			instructions = append(instructions, NewSqlInc(tree.Root.OpValue, tree.Root.Next.OpValue, tree.Root.Next.Next.OpValue))
			break
		}
	}
	return instructions, nil
}
