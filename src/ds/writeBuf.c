#include "writeBuf.h"
#include "string.h"
#include "src/ds/blkOps.h"
#include "src/ds/freeSegment.h"

extern Segment *segment;
extern InodeMap *mp;

Segment initializeWriteBuf(void){
    return Segment segment{
        .ssPos(0),
        .flashFreePos(0),
        .filePos(2),
        .inodePos(FILEBLOCKSIZE/INODESIZE),
        .seg(malloc(SEGMENTLENGTH))
    };
}

static inline void pushGetNewSeg(){
    for(int i = 0 ;i < MAXINDEX;i++){
        const ListofPhy *p = (mp->mp)[i];
        while(p->next){
            *(uint32_t *)(segment->seg + nextpos) = (p->next).key;
            *(uint32_t *)(segment->seg + nextpos + 4) = (p->next).value;
            nextpos += 4*2;
        }
    }
    writeSegment(segment->flashFreePos,segment->seg)egment
    segment->flashFreePos = allocateNewSegment();
    segment->ssPos = 0;
    segment->filePos = 2;
    segment->inodePos = FILEBLOCKSIZE/INODESIZE;
}

static int32_t writeInode(Inode i){
    memcpy((segment->seg + segment->inodePos),(uint8_t*)&i,INODESIZE);
    uint32_t res = segment->inodePos;
    uint32_t *searched = mapInode(i.inode_num);
    //查不到这个inode，说明是新建的
    if(!searched){
        insertInode(i.inode,res);
    } else{
        *searched = res;
    }
    segment->inodePos += INODESIZE;
    if(segment->inodePos % FILEBLOCKSIZE == 0){
        uint32_t nextpos = max(segment->inodePos,FILEBLOCKSIZE * (segment->filePos+1));
        uint32_t blks = ((mp->size - 1) / FILEBLOCKSIZE + 1);
        //段已满，写入flash并拿新段
        if(nextpos/FILEBLOCKSIZE == SEGMENTLENGTH/FILEBLOCKSIZE - blks){
            pushGetNewSeg();
        } else{
            segment->inodePos = nextpos;
        }
    }
    return res;
}

static inline uint32_t writeOneBlk(uint32_t *sz,Inode *i,uint8_t *content,uint32_t offset){
    SSEntry *newSS = (SSEntry *)(segment->seg) + segment->ssPos;
    newSS->offset = segment->filePos;
    newSS->inode_num = i->inode_num;
    uint32_t writeP = segment->flashFreePos + newSS->offset;
    //写块
    uint32_t writeNum = min(sz,FILEBLOCKSIZE - offset);
    memcpy((segment->seg) + FILEBLOCKSIZE * (segment->filePos) + offset,content,writeNum);
    content += writeNum;
    i->size += writeNum;
    //判断是否段已满
    uint32_t nextpos = max(segment->inodePos/FILEBLOCKSIZE,segment->filePos+1);
    uint32_t blks = ((mp->size - 1) / FILEBLOCKSIZE + 1);
    //段已满，写入flash并拿新段
    if(nextpos == SEGMENTLENGTH/FILEBLOCKSIZE - blks){
        pushGetNewSeg();
    } else{
        segment->filePos = nextpos;
    }
    *sz -= writeNum;
    return writeP;
}

static uint32_t writelevel2Blk(uint32_t level2blk,uint32_t *blk,uint32_t posIn2level,uint32_t offset,uint32_t *sz,Inode *i,uint8_t *content){
    uint8_t level2Block[FILEBLOCKSIZE];
    readFileBlock(level2blk,level2Block);
    if(offset != 0){
        uint8_t readContent[FILEBLOCKSIZE];
        readFileBlock(*((uint32_t *)level2Block + posIn2level),readContent);
        memcpy((segment->seg) + FILEBLOCKSIZE * (segment->filePos),readContent,offset);
        i->size += offset;
    }
    while(*sz > 0 && posIn2level < FILEBLOCKSIZE/sizeof(uint32_t)){
        uint32_t writeP = writeOneBlk(sz,i,content,offset);
        *blk += 1;
        *((uint32_t *)level2Block + posIn2level) = writeP;
        offset = 0;
        posIn2level += 1;
    }
    if(*sz <= 0){
        uint32_t fileblocksize = FILEBLOCKSIZE;
        uint32_t writeP = writeOneBlk(&fileblocksize,i,level2Block,0);
        return writeP;
    }
}

int32_t writeToBuf(Inode i,uint32_t lseekPos,uint8_t *content,uint32_t sz){
    //只新建inode
    if(content == NULL){
        return writeInode(i);
    }

    // writeInode(i);
    uint32_t blk = lseekPos / FILEBLOCKSIZE,offset = lseekPos % FILEBLOCKSIZE;
    //从直接块开始修改
    if(blk < MAXDIRECT){
        if(offset != 0){
            uint8_t readContent[FILEBLOCKSIZE];
            readFileBlock(blk,readContent);
            memcpy((segment->seg) + FILEBLOCKSIZE * (segment->filePos),readContent,offset);
            i.sz += offset;
        }
        while(sz > 0 && blk < MAXDIRECT){
            uint32_t writeP = writeOneBlk(&sz,&i,content,offset);
            i->indexes[blk] = writeP;
            blk += 1;
            offset = 0;
        }
        if(sz <= 0){
            return writeInode(i);
        }
    }
    //写入的位置超过了直接块的位置,但还没到三级块
    if(blk >= MAXDIRECT && blk < MAXDIRECT + FILEBLOCKSIZE/sizeof(uint32_t)){
        uint32_t res = writelevel2Blk(i.indexes[MAXDIRECT],&blk,blk - MAXDIRECT,offset,&sz,&i,content);
        offset = 0;
        i.indexes[MAXDIRECT] = res;
        if(sz <= 0) return writeInode(*i);
    }
    //写入位置就在三级块范围内
    if(blk >= FILEBLOCKSIZE/sizeof(uint32_t) + MAXDIRECT){
        uint8_t level3Block[FILEBLOCKSIZE];
        readFileBlock(i.indexes[MAXDIRECT+1],level3Block);
        while(sz > 0){
            uint32_t remap = blk - MAXDIRECT - FILEBLOCKSIZE/sizeof(uint32_t);
            uint32_t posIn3level = remap / (FILEBLOCKSIZE / sizeof(uint32_t));
            uint32_t m = remap % (FILEBLOCKSIZE / sizeof(uint32_t));
            uint32_t res = writelevel2Blk(*(uint32_t*)(level3Block + posIn3level),
                           &blk,
                           m,
                           offset,
                           &sz,
                           &i,
                           content
                          );
            *(uint32_t*)(level3Block + posIn3level) = res;
            posIn3level += 1;
        }
    }
}

