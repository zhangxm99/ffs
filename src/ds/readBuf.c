#include "readBuf.h"
#include "blkOps/blkOps.h"
#include <stdlib.h>
#include <string.h>

#include "writeBuf.h"

ReadBuffer *head = NULL;

extern Segment *segment;  

uint8_t* findBlockInWriteBuffer(uint32_t inode_num, uint32_t offset) {
    SSEntry *entries = (SSEntry *)(segment->seg) + 1;
    uint32_t count = *(uint32_t *)(segment->seg);  // 假设 filePos 表示当前段中的条目数

    for (uint32_t i = 0; i < count; i++) {
        if (entries[i].inode_num == inode_num && entries[i].blkAttribute == 1 && entries[i].offset*FILEBLOCKSIZE == offset) {
            // 假设每个条目后面紧跟着它的数据块
            return (uint8_t *)(segment->seg) + FILEBLOCKSIZE *(i+1);
        }
    }

    return NULL;  // 没有找到匹配的条目
}

void initializeReadBuffer(void) {
    head = NULL;  // 初始化头指针为NULL，表示缓冲区为空
}

uint8_t* findBlockInReadBuffer(uint32_t inode_num, uint32_t offset) {
    // 首先检查写缓存区
    uint8_t* data = findBlockInWriteBuffer(inode_num, offset);
    if (data != NULL) {
        return data;  // 在写缓存中找到了数据，直接返回
    }

    // 检查读缓存区
    ReadBuffer *current = head;
    while (current != NULL) {
        if (current->inode_num == inode_num && current->offset == offset) {
            return current->data;  // 在读缓存中找到匹配的块，返回数据指针
        }
        current = current->next;
    }

    // 如果在缓存中未找到，从磁盘读取
    return readBlockFromDisk(inode_num, offset);
}

void readBlockToBuffer(uint32_t block_num, uint32_t offset) {
    uint8_t *data = (uint8_t *)malloc(FILEBLOCKSIZE);
    if (data == NULL) {
        // 处理内存分配失败的情况
        exit(1);
    }

    if (readFileBlock(block_num, (char *)data) != 0) {
        // 读取磁盘块失败
        free(data);
        return;
    }

    // 创建新的缓存节点
    ReadBuffer *newNode = (ReadBuffer *)malloc(sizeof(ReadBuffer));
    if (newNode == NULL) {
        // 处理内存分配失败的情况
        free(data);
        exit(1);
    }

    newNode->inode_num = block_num;  // 这里应该是inode编号，需要根据实际情况调整
    newNode->block_num = block_num;
    newNode->offset = offset;
    newNode->data = data;
    newNode->next = head;
    head = newNode;  // 插入到链表头部
}