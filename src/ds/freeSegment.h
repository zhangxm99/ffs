#ifndef FREESEGMENT_H
#define FREESEGMENT_H
typedef struct FreeSegment{
    //空闲号，单位是FILEBLOCKSIZE，但必须可以整除SEGMENTLENGTH
    uint32_t freeNum;
    struct FreeSegment *next; 
} FreeSegment;

//初始化，对全盘段依次进行扫描，将空闲段整理出来，只要整理够5个段就停手
FreeSegment initializeFreeSegment(void);

//分配一个新段给出去，如果链上没有了就继续扫盘整理，只要整理够5个段就停手
uint32_t allocateNewSegment(void);

#endif

