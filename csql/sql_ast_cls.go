package csql

import (
	"csdb-teach/conf"
	"errors"
	stack "github.com/duke-git/lancet/v2/datastructure/stack"
	"strings"
)

type ASTTree struct {
	se       *SqlEngine
	Root     *ASTNode
	Tokens   []*Token
	Children []*ASTTree
}

type ASTNode struct {
	Token   *Token
	Next    *ASTNode
	OpValue uint16
	OpType  uint8
	OpBind  uint8
}

func NewASTTree(se *SqlEngine, tokens []*Token) *ASTTree {
	var tree = new(ASTTree)
	tree.se = se
	tree.Tokens = tokens
	tree.Root = NewASTNode(tokens[0])
	return tree
}

func NewASTNode(token *Token) *ASTNode {
	var node = new(ASTNode)
	node.Token = token
	node.OpValue = token.OpValue
	node.OpType = token.OpType
	node.OpBind = token.OpBind
	return node
}

func (a *ASTTree) Build() (*ASTTree, error) {
	switch a.Root.Token.OpValue {
	case OpCodeCreate:
		var omCode = a.se.vm.om[strings.ToUpper(a.Tokens[1].Value)]
		if omCode == 0 {
			return nil, errors.New(conf.ErrSyntax)
		}
		if a.Tokens[1].OpType == OpTypeObject {
			if a.Tokens[1].OpValue == OmCodeDatabase {
				a.Root.Next = NewASTNode(a.Tokens[1])
				if a.Tokens[2].OpType == OpTypeData {
					a.Root.Next.Next = NewASTNode(a.Tokens[2])
				} else {
					return nil, errors.New(conf.ErrSyntax)
				}
			} else if a.Tokens[1].OpValue == OmCodeTable {
				return a.buildCreateTableTree()
			}
		} else {
			return nil, errors.New(conf.ErrSyntax)
		}
		break
	case OpCodeInsert:
		if a.Tokens[1].OpValue == OpCodeInto {
			return a.buildInsertTableTree()
		} else {
			return nil, errors.New(conf.ErrSyntax)
		}
	case OpCodeUse:
		a.Root.Next = NewASTNode(a.Tokens[1])
		break
	}
	return a, nil
}

func (a *ASTTree) buildCreateTableTree() (*ASTTree, error) {
	// 建表语句
	r := NewASTTree(a.se, a.Tokens)
	r.Root.Next = NewASTNode(a.Tokens[1])
	r.Root.Next.Next = NewASTNode(a.Tokens[2])
	// 拆分字段
	var column = make([]*Token, 0)
	var bracketStack = stack.NewLinkedStack[string]()
	for _, token := range a.Tokens[3:] {
		switch token.Value {
		case "(":
			bracketStack.Push(token.Value)
			continue
		case ")":
			if peak, err := bracketStack.Peak(); err == nil {
				if *peak == "(" {
					_, err = bracketStack.Pop()
					if err != nil {
						return nil, err
					}
					if bracketStack.IsEmpty() && len(column) > 0 {
						tree := NewASTTree(a.se, column)
						p := tree.Root
						for i := 1; i < len(tree.Tokens); i++ {
							p.Next = NewASTNode(tree.Tokens[i])
							p = p.Next
						}
						r.Children = append(r.Children, tree)
						return r, nil
					}
				}
			} else {
				return nil, err
			}
			break
		case ",":
			if len(column) > 0 {
				tree := NewASTTree(a.se, column)
				p := tree.Root
				for i := 1; i < len(tree.Tokens); i++ {
					p.Next = NewASTNode(tree.Tokens[i])
					p = p.Next
				}
				r.Children = append(r.Children, tree)
				column = make([]*Token, 0)
			}
			break
		default:
			column = append(column, token)
			break
		}
	}
	return nil, nil
}

func (a *ASTTree) buildInsertTableTree() (*ASTTree, error) {
	// 删除INTO（无意义）
	a.Tokens = append(a.Tokens[:1], a.Tokens[2:]...)
	r := NewASTTree(a.se, a.Tokens)
	// 获取表名
	r.Root.Next = NewASTNode(a.Tokens[1])
	// 字段名列表
	var columnIndex = -1
	var columnCount = 0
	// 值列表
	var valueIndex = -1
	var valueCount = 0
	if r.Tokens[2].Type == TokenTypeSymbol && r.Tokens[2].Value == "(" {
		columnIndex = 3
	} else if r.Tokens[2].Type == TokenTypeKeyword && r.Tokens[2].Value == KwValues {
		valueIndex = 3
	}
	if columnIndex > 0 {
		var i = columnIndex
		for ; r.Tokens[i].Value != ")"; i++ {
			if r.Tokens[i].Value == "," {
				continue
			} else {
				r.Children = append(r.Children, NewASTTree(r.se, []*Token{r.Tokens[i]}))
				columnCount++
			}
		}
		valueIndex = i + 3
	}
	var entryIndex = 0
	for i := valueIndex; r.Tokens[i].Value != ")"; i++ {
		if r.Tokens[i].Value == "," {
			continue
		} else {
			valueCount++
			if len(r.Children) > 0 && len(r.Children) == columnCount {
				r.Children[entryIndex].Root.Next = NewASTNode(r.Tokens[i])
				entryIndex++
			} else {
				r.Children = append(r.Children, NewASTTree(r.se, []*Token{r.Tokens[i]}))
			}
		}
	}
	return r, nil
}
