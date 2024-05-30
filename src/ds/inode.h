#ifndef INODE_H
#define INODE_H
#include <stdint.h>
#define MAXDIRECT 11
#define MAXSTRINGLENGTH 64
#define INODESIZE sizeof(Inode)

//128byte
struct Inode{
    uint32_t inode_num;
    char name[MAXSTRINGLENGTH];
    //高1位标志是否为目录，低9位为权限标注
    //如果这是一个目录，则块中只包含inode号（这里考虑如果直接包含（inode，phy）对会怎么样）
    uint16_t isDirAndPermission;
    uint16_t ownerID;
    uint32_t size;
    //MAXDIRECT个一级块，1个二级块，1个三级块
    uint32_t indexes[MAXDIRECT+2];
} __attribute__((packed));

typedef struct Inode Inode;

#endif

