#include "blkOps.h"

extern FlashDev dev;

int32_t writeFlashBlock(uint32_t blk,char *contents){
    for(int i = 0;i < FLASHBLOCKSIZE;i++){
        dev[blk*FILEBLOCKSIZE + i] = contents[i];
    }
    return 0;
}

int32_t readFileBlock(uint32_t blk,char *res){
    for(int i = 0;i < FILEBLOCKSIZE;i++){
        res[i] = dev[blk*FILEBLOCKSIZE + i];
    }
    return 0;
}

int32_t writeSegment(uint32_t startBlock,char *contents){
    for(int i = 0;i < SEGMENTLENGTH / FLASHBLOCKSIZE;i++){
        if(writeFlashBlock(dev,startBlock + FLASHBLOCKSIZE/FILEBLOCKSIZE,contents+i*FLASHBLOCKSIZE) != 0){
            return -1;
        }
    }
    return 0;
}

int32_t readSegement(uint32_t startBlock,char *res){
    for(int i = 0;i < SEGMENTLENGTH / FILEBLOCKSIZE;i++){
        if(readFileBlock(dev,startBlock + i,res + i*FILEBLOCKSIZE) != 0){
            return -1;
        }
    }
    return 0;
}
