#ifndef BLKOPS_H
#define BLKOPS_H

#include <stdint.h>
#define MEMFLASHSIZE 512*1024*1024
//读取的大小（flash的page）等于文件块的大小
#define FILEBLOCKSIZE 4*1024
//写入的大小就是flash的擦除块的大小
#define FLASHBLOCKSIZE 256*1024
#define SEGMENTLENGTH 2*1024*1024

typedef uint8_t FlashDev[MEMFLASHSIZE];

//写入一个段，给定段起始地址和内容，注意这里写入长度一定是等于段长度的，所以不用给定写入长度参数
//这里的startBlock对应的是FILEBLOCK，并且startBlock可以整除FLASHBLOCKSIZE
int32_t writeSegment(uint32_t startBlock,char *contents);

int32_t readSegement(uint32_t startBlock,char *res);

int32_t writeFlashBlock(uint32_t blk,char *contents);

int32_t readFileBlock(uint32_t blk,char *res);

#endif