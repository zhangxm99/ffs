#ifndef INODEMAP_H
#define INODEMAP_H
#include <stdint.h>
#include "src/blkOps/blkOps.h"
#define MAXINDEX 1024

typedef struct ListofPhy{
    uint32_t key;
    uint32_t value;
    struct ListofPhy *next;
} ListofPhy;

typedef struct{
    ListofPhy* mp[MAXINDEX];
    uint32_t size;
}InodeMap;

InodeMap generateInodeMap(FlashDev);

//在映射表中查找inode号对应的位置，如果不存在则返回-1
uint32_t* mapInode(uint32_t);

//在映射表中插入一个inode,phy,offset键值对
void insertInode(uint32_t ,uint32_t);

//在映射表中删除一项
void deleteInode(uint32_t );

#endif
