package csql

import (
	"csdb-teach/conf"
	"errors"
	"strings"
)

type ASTTree struct {
	se     *SqlEngine
	Root   *ASTNode
	Tokens []*Token
}

type ASTNode struct {
	Token   *Token
	Next    *ASTNode
	OpValue uint8
	OpType  uint8
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
			a.Root.Next = NewASTNode(a.Tokens[1])
			if a.Tokens[2].OpType == OpTypeData {
				a.Root.Next.Next = NewASTNode(a.Tokens[2])
			} else {
				return nil, errors.New(conf.ErrSyntax)
			}
		} else {
			return nil, errors.New(conf.ErrSyntax)
		}
		break
	case OpCodeUse:
		a.Root.Next = NewASTNode(a.Tokens[1])
		break
	}
	return a, nil
}
