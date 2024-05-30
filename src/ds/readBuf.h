#ifndef READBUF_H
#define READBUF_H


//当调用read时，首先去写缓存中查找inode块是否存在，如果存在则直接返回
//否则去写入缓冲区中查找是否存在，如果存在则返回
//否则去读磁盘，拿到新块以后如果缓存区未满则放到最前面，否则进行块替换

typedef struct {
    uint32_t inode_num;  // inode编号
    uint32_t block_num;  // 块编号
    uint32_t offset;     // 文件中的偏移量
    uint8_t *data;       // 数据指针
    struct ReadBuffer *next;  // 链表结构，用于链接缓存中的下一个块
} ReadBuffer;

uint8_t* findBlockInReadBuffer(uint32_t inode_num, uint32_t offset);

void readBlockToBuffer(uint32_t block_num, uint32_t offset);

#endif