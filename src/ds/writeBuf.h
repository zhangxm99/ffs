#include "src/ds/inode.h"

typedef struct{
    //指示块位置，单位是块
    uint16_t offset;
    //指示所属文件号
    uint16_t inode_num;
}SSEntry;

typedef struct{
    //段总结表下一个要写的位置
    uint32_t ssPos;
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

//向flash中写入数据和对应的inode，如果缓冲区size为0，则说明只新建文件，不分配块
//失败则返回-1
//调用该函数时必须保证lseekPos不超过文件大小
int32_t writeToBuf(Inode i,uint32_t lseekPos,uint8_t *content,uint32_t sz);