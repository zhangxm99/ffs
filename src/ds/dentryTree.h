#ifndef DENTRYTREE_H
#define DENTRYTREE_H
#define STRINGLENGTH 64
#include <stdint.h>
#include "src/blkOps/blkOps.h"
#include "src/ds/inodeMap.h"

typedef struct ListofTreeNode{
    struct DentryTree node;
    struct ListofTreeNode *next;
} ListofTreeNode;

typedef struct DentryTree{
    uint16_t isDirAndPermission;
    char filename[STRINGLENGTH];
    uint32_t inode_num;
    ListofTreeNode *next_level;
} DentryTree;

//根据磁盘上的根结点构建目录树
DentryTree generateTree();

//通过Inode号拿到inode
inline Inode getInode(uint32_t inode_num);

//根据路径搜索文件，如果找到文件并在搜索过程中权限匹配则返回对应的inode
//如果权限不够则返回-1，如果文件不存在或路径不合法则返回-2,如果要创建但已经存在且带有excl则返回-3
int32_t searchInode(const char *path,uint8_t permission,uint32_t isCreat,uint32_t is_excl,uint32_t is_trunc);

#endif
