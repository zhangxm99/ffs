#include "writeBuf.h"
#include "string.h"
#include "src/ds/blkOps.h"
#include "src/ds/freeSegment.h"

extern Segment *segment;
extern InodeMap *imap;

Segment initializeWriteBuf(void){
    Segment s{
        .flashFreePos(allocateNewSegment()),
        .filePos(2),
        .inodePos(FILEBLOCKSIZE/INODESIZE),
        .seg(malloc(SEGMENTLENGTH))
    };
    *(uint32_t *)(s.seg) = 0;
    return s;
}

static inline void pushGetNewSeg(Segment *segment,uint32_t nextpos){
    for(int i = 0 ;i < MAXINDEX;i++){
        const ListofPhy *p = (imap->mp)[i];
        while(p->next){
            *(uint32_t *)(segment->seg + nextpos) = (p->next).key;
            *(uint32_t *)(segment->seg + nextpos + 4) = (p->next).value;
            nextpos += 4*2;
        }
    }
    writeSegment(segment->flashFreePos,segment->seg);
    *(uint32_t *)(segment->seg) = 0;
    segment->flashFreePos = allocateNewSegment();
    segment->filePos = 2;
    segment->inodePos = FILEBLOCKSIZE;
}

int32_t writeInode(Segment *segment,Inode i){
    //段已满，写入flash并拿新段
    if(segment->inodePos/FILEBLOCKSIZE == SEGMENTLENGTH/FILEBLOCKSIZE - blks){
        pushGetNewSeg(segment,nextpos);
    }
    memcpy((segment->seg + segment->inodePos),(uint8_t*)&i,INODESIZE);
    uint32_t res = segment->inodePos;
    uint32_t *searched = mapInode(i.inode_num);
    SSEntry *newSS = (SSEntry *)(segment->seg) + (segment->inodePos/FILEBLOCKSIZE);
    //新占块，块数需要加一
    if(newSS->offset == 0) *(uint32_t *)(segment->seg) += 1;
    newSS ->blkAttribute = 0;
    newSS->offset += 1;
    //查不到这个inode，说明是新建的
    if(!searched){
        insertInode(i.inode,res);
    } else{
        *searched = res;
    }
    segment->inodePos += INODESIZE;
    if(segment->inodePos % FILEBLOCKSIZE == 0){
        uint32_t nextpos = max(segment->inodePos,FILEBLOCKSIZE * (segment->filePos+1));
        uint32_t blks = ((imap->size - 1) / FILEBLOCKSIZE + 1);
        
        ((SSEntry *)(segment->seg) + (nextpos/FILEBLOCKSIZE)) -> blkAttribute = 0;
        ((SSEntry *)(segment->seg) + (nextpos/FILEBLOCKSIZE)) -> offset = 0;
        segment->inodePos = nextpos;
    }
    return res;
}

inline uint32_t writeOneBlk(Segment *segment, uint32_t level ,uint32_t *sz,uint32_t blk,Inode *i,uint8_t *content,uint32_t offset){
    SSEntry *newSS = (SSEntry *)(segment->seg) + segment->filePos;
    newSS->offset = blk;
    newSS->inode_num = i->inode_num;
    newSS->blkAttribute = level;
    uint32_t writeP = segment->flashFreePos + segment->filePos;
    //写块
    uint32_t writeNum = min(*sz,FILEBLOCKSIZE - offset);
    memcpy((segment->seg) + FILEBLOCKSIZE * (segment->filePos) + offset,content,writeNum);
    content += writeNum;
    i->size += writeNum;
    //判断是否段已满
    uint32_t nextpos = max(segment->inodePos/FILEBLOCKSIZE,segment->filePos+1);
    uint32_t blks = ((imap->size - 1) / FILEBLOCKSIZE + 1);
    //段已满，写入flash并拿新段
    if(nextpos == SEGMENTLENGTH/FILEBLOCKSIZE - blks){
        pushGetNewSeg(segment,nextpos);
    } else{
        //新占块，块数需要加一
        *(uint32_t *)(segment->seg) += 1;
        segment->filePos = nextpos;
    }
    *sz -= writeNum;
    return writeP;
}

static uint32_t writeLevelM(
    //指示是更高级块中的第几个，如果是-1则表示这不是从更高级块中拿出来的，而是从inode的M级块中取出的
    uint32_t offset,
    uint32_t level,
    //blk指示在inode中的第几块
    uint32_t blk,
    Inode *i,
    uint32_t indexblock,
    uint32_t lseekPos,
    uint8_t *content,
    uint32_t *sz,
){
    if(level == 1) return writeOneBlk(segment,1,sz,blk,i,content,lseekPos);

    uint32_t stepSize = FILEBLOCKSIZE;
    for(int i = 0;i < level-2;i++) stepSize *= (FILEBLOCKSIZE / sizeof(uint32_t));

    uint32_t nxt = lseekPos / stepSize;
    uint8_t *indexes = (uint8_t *)malloc(FILEBLOCKSIZE);
    readFileBlock(indexblock,indexes);
    uint32_t first = lseekPos - nxt*FILEBLOCKSIZE;
    while(*sz > 0 && nxt < (FILEBLOCKSIZE / sizeof(uint32_t))){
        *((uint32_t *)indexes + nxt)  = writeLevelM(nxt,level-1,blk,*((uint32_t *)indexes + nxt),first,content,sz);
        blk += stepSize / FILEBLOCKSIZE;
        first = 0;
        nxt += 1;
    }
    uint32_t fileblocksize = FILEBLOCKSIZE;
    uint32_t writeP = writeOneBlk(segment,-level,&fileblocksize,offset,i,indexes,0);

    free(indexes);

    return writeP;
}

//下面的是非递归老方法
// static uint32_t writelevel2Blk(uint32_t level2blk,uint32_t *blk,uint32_t posIn2level,uint32_t offset,uint32_t *sz,Inode *i,uint8_t *content){
//     uint8_t level2Block[FILEBLOCKSIZE];
//     readFileBlock(level2blk,level2Block);
//     if(offset != 0){
//         uint8_t readContent[FILEBLOCKSIZE];
//         readFileBlock(*((uint32_t *)level2Block + posIn2level),readContent);
//         memcpy((segment->seg) + FILEBLOCKSIZE * (segment->filePos),readContent,offset);
//         i->size += offset;
//     }
//     while(*sz > 0 && posIn2level < FILEBLOCKSIZE/sizeof(uint32_t)){
//         uint32_t writeP = writeOneBlk(sz,i,content,offset);
//         *blk += 1;
//         *((uint32_t *)level2Block + posIn2level) = writeP;
//         offset = 0;
//         posIn2level += 1;
//     }
//     if(*sz <= 0){
//         uint32_t fileblocksize = FILEBLOCKSIZE;
//         uint32_t writeP = writeOneBlk(&fileblocksize,i,level2Block,0);
//         return writeP;
//     }
// }

int32_t writeToBuf(Inode i,uint32_t lseekPos,uint8_t *content,uint32_t sz){
    //只新建inode
    if(content == NULL){
        return writeInode(segment,i);
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
            uint32_t writeP = writeOneBlk(segment,&sz,&i,content,offset);
            i->indexes[blk] = writeP;
            blk += 1;
            offset = 0;
        }
    }
    //写入的位置超过了直接块的位置,但还没到三级块
    if(sz > 0 && blk >= MAXDIRECT && blk < MAXDIRECT + FILEBLOCKSIZE/sizeof(uint32_t)){
        i.indexes[MAXDIRECT] = writeLevelM(-1,2,blk,&i,i.indexes[MAXDIRECT],lseekPos-MAXDIRECT*FILEBLOCKSIZE,content,sz);
        //下面的是非递归老方法
        // uint32_t res = writelevel2Blk(i.indexes[MAXDIRECT],&blk,blk - MAXDIRECT,offset,&sz,&i,content);
        // offset = 0;
        // i.indexes[MAXDIRECT] = res;
        // if(sz <= 0) return writeInode(*i);
    }
    //写入位置就在三级块范围内
    if(sz > 0 && blk >= FILEBLOCKSIZE/sizeof(uint32_t) + MAXDIRECT){
        uint32_t newp = lseekPos-MAXDIRECT*FILEBLOCKSIZE-(FILEBLOCKSIZE/sizeof(uint32_t));
        i.indexes[MAXDIRECT + 1] = writeLevelM(-1,3,blk,&i,i.indexes[MAXDIRECT+1],newp,content,sz);

        //下面的是非递归老方法
        // uint8_t level3Block[FILEBLOCKSIZE];
        // readFileBlock(i.indexes[MAXDIRECT+1],level3Block);
        // //注意这里没有做大小限制，默认不会超过文件规定大小，如果超过了则会直接错误
        // while(sz > 0){
        //     uint32_t remap = blk - MAXDIRECT - FILEBLOCKSIZE/sizeof(uint32_t);
        //     uint32_t posIn3level = remap / (FILEBLOCKSIZE / sizeof(uint32_t));
        //     uint32_t m = remap % (FILEBLOCKSIZE / sizeof(uint32_t));
        //     uint32_t res = writelevel2Blk(*(uint32_t*)(level3Block + posIn3level),
        //                    &blk,
        //                    m,
        //                    offset,
        //                    &sz,
        //                    &i,
        //                    content
        //                   );
        //     *(uint32_t*)(level3Block + posIn3level) = res;
        //     posIn3level += 1;
        // }
        // if(*sz <= 0){
        //     uint32_t fileblocksize = FILEBLOCKSIZE;
        //     i.indexes[MAXDIRECT+1] = writeOneBlk(&fileblocksize,&i,level3Block,0);
        //     return writeInode(*i);
        // }
    }
    //注意这里没有做大小限制，默认不会超过文件规定大小，如果超过了则会直接错误
    if(sz > 0) exit(1);
    return writeInode(segment,i);
}

