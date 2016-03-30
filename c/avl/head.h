#ifndef HEAD_H_
#define HEAD_H_

#include <stdio.h>
#include <stdlib.h>

typedef int ElementType;
typedef struct AvlNode              // AVL树的节点
{
    ElementType data;
    struct AvlNode *left;           // 左孩子
    struct AvlNode *right;          // 右孩子
    int Height;
}*Position,*AvlTree;

AvlTree MakeEmpty(AvlTree T);
Position Find(ElementType x,AvlTree T);
Position FindMin(AvlTree T);
Position FindMax(AvlTree T);
AvlTree  Insert(ElementType x,AvlTree T);
AvlTree  Delete(ElementType x,AvlTree T);
ElementType Retrieve(Position P);
void Display(AvlTree T);

#endif /* HEAD_H_ */
