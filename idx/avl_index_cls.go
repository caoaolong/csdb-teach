package idx

import (
	"csdb-teach/cfs"
	"csdb-teach/row"
)

type AVLNode struct {
	Value  *row.Index
	Left   *AVLNode
	Right  *AVLNode
	Height int
}

type AVLTree struct {
	Root *AVLNode
}

func NewAVLTree() *AVLTree {
	var tree = new(AVLTree)
	return tree
}

// getBalanceFactor 计算节点的平衡因子
func (n *AVLNode) getBalanceFactor() int {
	if n == nil {
		return 0
	}
	return n.Left.getHeight() - n.Right.getHeight()
}

func (n *AVLNode) getHeight() int {
	if n == nil {
		return 0
	}
	return n.Height
}

// rightRotate 右旋转操作
func rightRotate(y *AVLNode) *AVLNode {
	x := y.Left
	T2 := x.Right

	// 旋转
	x.Right = y
	y.Left = T2

	// 更新高度
	y.Height = max(y.Left.getHeight(), y.Right.getHeight()) + 1
	x.Height = max(x.Left.getHeight(), x.Right.getHeight()) + 1

	return x
}

// leftRotate 左旋转操作
func leftRotate(x *AVLNode) *AVLNode {
	y := x.Right
	T2 := y.Left

	// 旋转
	y.Left = x
	x.Right = T2

	// 更新高度
	x.Height = max(x.Left.getHeight(), x.Right.getHeight()) + 1
	y.Height = max(y.Left.getHeight(), y.Right.getHeight()) + 1

	return y
}

// insert 插入操作
func (t *AVLTree) insert(root *AVLNode, value *row.Index) *AVLNode {
	// 1. 执行标准的二叉搜索树插入
	if root == nil {
		return &AVLNode{Value: value, Height: 1}
	}

	if value.Value < root.Value.Value {
		root.Left = t.insert(root.Left, value)
	} else if value.Value > root.Value.Value {
		root.Right = t.insert(root.Right, value)
	} else {
		return root // 不允许插入重复值
	}

	// 2. 更新当前节点的高度
	root.Height = max(root.Left.getHeight(), root.Right.getHeight()) + 1

	// 3. 检查并恢复平衡
	balanceFactor := root.getBalanceFactor()

	// 左子树比右子树高，发生了不平衡
	// 左左情况 (Left-Left)
	if balanceFactor > 1 && value.Value < root.Left.Value.Value {
		return rightRotate(root)
	}

	// 右右情况 (Right-Right)
	if balanceFactor < -1 && value.Value > root.Right.Value.Value {
		return leftRotate(root)
	}

	// 左右情况 (Left-Right)
	if balanceFactor > 1 && value.Value > root.Left.Value.Value {
		root.Left = leftRotate(root.Left)
		return rightRotate(root)
	}

	// 右左情况 (Right-Left)
	if balanceFactor < -1 && value.Value < root.Right.Value.Value {
		root.Right = rightRotate(root.Right)
		return leftRotate(root)
	}

	return root
}

// Insert 插入到树中
func (t *AVLTree) Insert(value *row.Index) {
	t.Root = t.insert(t.Root, value)
}

// 中序遍历
func inOrderWrite(root *AVLNode, page *cfs.Page) {
	if root != nil {
		inOrderWrite(root.Left, page)
		// 写入
		page.WriteMemory(root.Value.Encode(), false)
		inOrderWrite(root.Right, page)
	}
}

func (t *AVLTree) Write(page *cfs.Page) {
	inOrderWrite(t.Root, page)
}
