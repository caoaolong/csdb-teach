package unit

import (
	"csdb-teach/cfs"
	"csdb-teach/conf"
	"csdb-teach/idx"
	"csdb-teach/row"
	"fmt"
	"testing"
)

// TreeNode 定义二叉树的节点
type TreeNode struct {
	Value  int
	Left   *TreeNode
	Right  *TreeNode
	Height int
}

// AVLTree 定义AVL树结构
type AVLTree struct {
	Root *TreeNode
}

// 获取节点的高度
func (n *TreeNode) getHeight() int {
	if n == nil {
		return 0
	}
	return n.Height
}

// 计算节点的平衡因子
func (n *TreeNode) getBalanceFactor() int {
	if n == nil {
		return 0
	}
	return n.Left.getHeight() - n.Right.getHeight()
}

// 右旋转操作
func rightRotate(y *TreeNode) *TreeNode {
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

// 左旋转操作
func leftRotate(x *TreeNode) *TreeNode {
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

// 获取最大值
func max(a, b int) int {
	if a > b {
		return a
	}
	return b
}

// 插入操作
func (t *AVLTree) insert(root *TreeNode, value int) *TreeNode {
	// 1. 执行标准的二叉搜索树插入
	if root == nil {
		return &TreeNode{Value: value, Height: 1}
	}

	if value < root.Value {
		root.Left = t.insert(root.Left, value)
	} else if value > root.Value {
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
	if balanceFactor > 1 && value < root.Left.Value {
		return rightRotate(root)
	}

	// 右右情况 (Right-Right)
	if balanceFactor < -1 && value > root.Right.Value {
		return leftRotate(root)
	}

	// 左右情况 (Left-Right)
	if balanceFactor > 1 && value > root.Left.Value {
		root.Left = leftRotate(root.Left)
		return rightRotate(root)
	}

	// 右左情况 (Right-Left)
	if balanceFactor < -1 && value < root.Right.Value {
		root.Right = rightRotate(root.Right)
		return leftRotate(root)
	}

	return root
}

// 插入到树中
func (t *AVLTree) Insert(value int) {
	t.Root = t.insert(t.Root, value)
}

// 中序遍历
func inOrder(root *TreeNode) {
	if root != nil {
		inOrder(root.Left)
		fmt.Printf("%d ", root.Value)
		inOrder(root.Right)
	}
}

func Test(t *testing.T) {
	// 创建一个 AVL 树
	tree := &AVLTree{}

	// 插入一些值
	values := []int{10, 20, 30, 40, 50, 25}
	for _, v := range values {
		tree.Insert(v)
	}
	// 打印树的中序遍历
	inOrder(tree.Root) // 10 20 25 30 40 50
}

func TestCreateIndex(t *testing.T) {
	var pf = new(cfs.PageFile)
	err := pf.Read("test1")
	if err != nil {
		t.Fatal(err)
	}
	page, err := pf.PageByType(conf.PageTypeIndex, 1)
	if err != nil {
		t.Fatal(err)
	}
	indexList := make([]*row.Index, 0)
	data := page.Raw()
	for i := 0; ; i += conf.IndexRowSize {
		entry := row.NewEmptyIndex().Read(data[i : i+conf.IndexRowSize])
		indexList = append(indexList, entry)
		if entry.Attr == 0 {
			break
		}
	}
	page.Clear()
	nums := []uint64{10, 20, 30, 40, 50, 25}
	tree := idx.NewAVLTree()
	for _, n := range nums {
		indexList = append(indexList, row.NewIndex(page.Index(), page.Offset(), n))
	}
	for _, e := range indexList {
		tree.Insert(e)
	}
	tree.Write(page)
	err = pf.Close()
	if err != nil {
		t.Fatal(err)
	}
}
