#ifndef WRITEBUF_H
#define WRITEBUF_H

#include "src/ds/inode.h"

struct SSEntry{
    //指示所属文件号（危险注意：这里应该是32位的，但是为了减小空间强行改成了16位，以后解决）
    uint16_t inode_num;
    //如果是FILEBLOCK则该项指示块偏移，如果是INODE该项指示块中inode个数
    //如果是索引块则指示该索引块在上层高级块中的偏移，如果是-1则表示该块没有上层，直接就是inode中存放的
    uint32_t offset;
    //指示是否为FILEBLOCK(1)还是INODEBLOCK（0）还是索引块（例如如果是1级索引块这个值就是-1）
    int8_t blkAttribute;
}__attribute__((packed)) ;

typedef struct SSEntry SSEntry;

typedef struct{
    //将要写入的flash位置
    uint32_t flashFreePos;
    //指使下一次要写入的位置，单位是文件块
    uint32_t filePos;
    //指示下一次inode要写入的位置，单位是字节
    uint32_t inodePos;
    //段放堆上，分别是头一个SS(占一个块)中间是数据尾巴是imap
    uint8_t *seg;
} Segment;

Segment initializeWriteBuf(void);

inline int32_t writeInode(Segment *segment,Inode i)

inline uint32_t writeOneBlk(Segment *segment,uint32_t level,uint32_t *sz,uint32_t blk,Inode *i,uint8_t *content,uint32_t offset);

//向flash中写入数据和对应的inode，如果缓冲区size为0，则说明只新建文件，不分配块
//失败则返回-1
//调用该函数时必须保证lseekPos不超过文件大小
int32_t writeToBuf(Inode i,uint32_t lseekPos,uint8_t *content,uint32_t sz);

#endif