package csql

import (
	"csdb-teach/cds"
	"csdb-teach/conf"
	list "github.com/duke-git/lancet/v2/datastructure/list"
	"slices"
	"strings"
	"sync"
	"unsafe"
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

func (s *SqlEngine) Database() *cds.Database {
	return (*cds.Database)(unsafe.Pointer(uintptr(s.vm.dpr)))
}

func (s *SqlEngine) Run(instructions []uint64) error {
	return s.vm.run(instructions)
}

func (s *SqlEngine) Close() {
	for _, v := range s.vm.pfm {
		_ = v.Close()
	}
}

func (s *SqlEngine) PushData(value string) uint8 {
	var index = len(s.vm.dm)
	s.vm.dm = append(s.vm.dm, OpData{
		Value: value,
	})
	return uint8(index)
}

func (s *SqlEngine) PushToken(token Token) {
	if token.Value == "" {
		return
	}
	var value = strings.ToUpper(token.Value)
	if is, v := conf.IsNumber(value); is {
		token.OpType = TokenTypeNumber
		token.OpValue = uint16(v)
		s.tokens.Push(token)
		return
	}
	var opType uint8 = 0
	var opValue uint16 = 0
	if slices.Contains(keywords, value) {
		opValue = uint16(s.vm.cm[value])
		opType = OpTypeCode
		if opValue == 0 {
			opValue = uint16(s.vm.om[value])
			opType = OpTypeObject
			if opValue == 0 {
				opType = OpTypeData
				opValue = uint16(s.PushData(token.Value))
			}
		}
	} else if slices.Contains(datatypes, value) {
		opValue = s.vm.dtm[value]
		opType = OpTypeAttr
	} else {
		opType = OpTypeData
		opValue = uint16(s.PushData(token.Value))
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
			s.PushToken(NewToken(string(char), TokenTypeDelimiter))
		case ',', '(', ')':
			s.PushToken(NewToken(value.String(), TokenTypeIdentifier))
			value.Reset()
			s.PushToken(NewToken(string(char), TokenTypeSymbol))
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

func (s *SqlEngine) Compile(trees []*ASTTree) ([]uint64, error) {
	var instructions = make([]uint64, 0)
	for _, tree := range trees {
		switch tree.Root.OpValue {
		case OpCodeCreate:
			// 创建数据库或者表
			instructions = append(instructions, NewSqlInc(
				uint8(tree.Root.OpValue), uint8(tree.Root.Next.OpValue), uint8(tree.Root.Next.Next.OpValue), 0))
			// 创建表的字段
			for _, child := range tree.Children {
				instructions = append(instructions, NewSqlInc(
					uint8(tree.Root.OpValue), uint8(tree.Root.Next.OpValue+1), uint8(child.Root.OpValue),
					child.Root.Next.OpValue))
				if len(child.Tokens) > 2 {
					instructions = append(instructions, NewSqlInc(OpCodeSet, OmCodeColumn, conf.SetTypeLength, child.Root.Next.Next.OpValue))
					if len(child.Tokens) > 3 {
						instructions = append(instructions, NewSqlInc(OpCodeSet, OmCodeColumn, conf.SetTypeBind, child.Root.Next.Next.Next.OpValue))
					}
				}
			}
			break
		case OpCodeUse:
			instructions = append(instructions, NewSqlInc(
				uint8(tree.Root.OpValue), 0, uint8(tree.Root.Next.OpValue), 0))
			break
		}
	}
	return instructions, nil
}
