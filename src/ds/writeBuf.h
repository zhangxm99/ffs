#include "src/ds/inode.h"

struct SSEntry{
    //指示所属文件号（危险注意：这里应该是32位的，但是为了减小空间强行改成了16位，以后解决）
    uint16_t inode_num;
    //如果是FILEBLOCK则该项指示块偏移，否则该项指示块中inode个数
    uint32_t offset;
    //指示是否为FILEBLOCK（以防是INODEBLOCK）
    uint8_t isFILE;
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

//向flash中写入数据和对应的inode，如果缓冲区size为0，则说明只新建文件，不分配块
//失败则返回-1
//调用该函数时必须保证lseekPos不超过文件大小
int32_t writeToBuf(Inode i,uint32_t lseekPos,uint8_t *content,uint32_t sz);