#ifndef OPENFILETABLE_H
#define OPENFILETABLE_H

#define MAXOPENFILE 1024
#include <stdint.h>

typedef struct OpenEntry{
    uint32_t filedesc;
    uint32_t offset;
    uint16_t permission;
    Inode i;
    struct OpenEntry *next;
} OpenEntry;

typedef struct{ 
    OpenEntry *dummyHead;
    //有序表，存储着已经分配的号段，采用这种存储方法的好处是分配新号只需要O(1),回收号只需要O(log n)
    OrderedPairTable used;

} OpenFileTable;

OpenFileTable initialize();

//放一个inode，要一个文件描述符号,如果打开文件项已满则返回-1
uint32_t putInodeInOpenFileTable(Inode,uint32_t);


#endif

