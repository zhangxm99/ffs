#include "blkOps/blkOps.h"
#include <stdint.h>
#include "ds/writeBuf.h"
#include "ds/dentryTree.h"
#include "ds/inode.h"
//定义第一个段起始位置
#define FIRSTSEGSTARTPOS SEGMENTLENGTH
//定义一次搜集的净块个数
#define THRESHOLD 5
//level2包含的一级块个数
#define LEVEL2SZ FILEBLOCKSIZE/sizeof(uint32_t)

//缓冲各个文件修改的信息
typedef struct ListofIndexes{
    Inode i;
    //动的二级块
    uint32_t *level2indexes;
    //动的三级块
    uint32_t *level3indexes;
    struct ListofIndexes *next;
} ListofIndexes;

// typedef struct ListofAliveInode{
//     Inode i;
//     struct ListofAliveInode *next;
// } ListofAliveInode;

extern FreeSegment *freeSeg;

static uint8_t* readIthBlock(Inode *i,uint32_t ithBlk){
    uint8_t *content = malloc(FILEBLOCKSIZE);
    if(ithBlk < MAXDIRECT){
        readFileBlock((i->indexes)[ithBlk],content);
    } else if(ithBlk < MAXDIRECT + LEVEL2SZ){
        uint8_t *indexes2 = malloc(FILEBLOCKSIZE);
        readFileBlock((i->indexes)[MAXDIRECT],indexes2);
        readFileBlock(*((uint32_t *)indexes2 + ithBlk - MAXDIRECT),content);
        free(indexes2);
    } else{
        uint8_t *indexes3 = malloc(FILEBLOCKSIZE);
        uint8_t *indexes2 = malloc(FILEBLOCKSIZE);
        readFileBlock((i->indexes)[MAXDIRECT+1],indexes3);
        readFileBlock(*((uint32_t *)indexes3 + (ithBlk - MAXDIRECT - LEVEL2SZ)/LEVEL2SZ),indexes2);
        readFileBlock(*((uint32_t *)indexes2 + (ithBlk - MAXDIRECT - LEVEL2SZ) % LEVEL2SZ),content);

        free(indexes3);
        free(indexes2);
    }
    return content;
    //!注意调用者必须free content
}

static uint8_t isAliveFileBlock(uint8_t *target,uint32_t inodeNum,uint32_t blk_off){
    Inode i = getInode(inodeNum);
    uint8_t *content = readIthBlock(&i,blk_off);
    for(int j = 0;j < FILEBLOCKSIZE;j++){
        if(*(target + j) != *(content + j)) return 0;
    }
    free(content);
    return 1;
}

static inline uint8_t isAliveInode(uint32_t inode_num, uint32_t inode_pos){
    uint32_t *p = mapInode(inode_num);
    if(*p == inode_pos) return 1;
    return 0;
}

static inline uint8_t isAliveIndexes(uint8_t *target,uint32_t inode_num,uint32_t level,uint32_t offset){
    Inode i = getInode(inodeNum);
    //说明就是inode中的块
    if(offset == -1){
        uint8_t *content = malloc(FILEBLOCKSIZE);
        readFileBlock((i.indexes)[-level-2 + MAXDIRECT],indexes);
        for(int j = 0;j < FILEBLOCKSIZE;j++){
            if(*(target + j) != *(content + j)) {
                free(content);
                return 0;
            }
        }
        free(content);
        return 1;
    } else{
        //在只有三级块的情况下，这说明这是三级块下的二级块
        uint32_t *thirdIndexes = malloc(FILEBLOCKSIZE);
        uint8 *content = malloc(FILEBLOCKSIZE);
        readFileBlock(i.indexes[MAXDIRECT + 1],thirdIndexes);
        readFileBlock(*(thirdIndexes + offset),secondIndexes);
        free(thirdIndexes);
        for(int j = 0;j < FILEBLOCKSIZE;j++){
            if(*(target + j) != *(content + j)) {
                free(content);
                return 0;
            }
        }
        free(content);
        return 1;
    }
}

static ListofIndexes* getListNode(ListofIndexes *indexesHeader,Inode *i){
    ListofIndexes *p = indexesHeader;
    while(p->next){
        if((p->next).i.inode_num == i.inode_num){
            break;
        }
    }
    //找不到，就新建
    if(!(p->next)){
        ListofIndexes *newI = malloc(ListofIndexes);
        newI->i = *i;
        newI->level2indexes = NULL;
        newI->next = NULL;
        p->next = newI;
    }
    p = p->next;
    return p;
}

static uint32_t countingSeg(){
    FreeSegment *p = freeSeg->next;
    uint32_t res = 0;
    while(p) {
        res++;
        p = p->next;
    }
    return res;
}

void swapSegment(uint32_t startPos){
    uint32_t initialLength = countingSeg();
    
    //读的缓冲段
    uint8_t *segBuf = (uint8_t *)malloc(SEGMENTLENGTH);
    //写的缓冲
    Segment writeSeg{
        .flashFreePos(startPos),
        .filePos(2),
        .inodePos(FILEBLOCKSIZE/INODESIZE),
        .seg(malloc(SEGMENTLENGTH))
    };
    *(uint32_t *)(writeSeg.seg) = 0;

    ListofIndexes *indexesHeader = malloc(ListofIndexes);
    // ListofAliveInode *inodesHeader = malloc(ListofAliveInode);


    for(int segPos = startPos;countingSeg() - initialLength < THRESHOLD;){
        //首先读入多段，然后对多段进行：
        //1）活块检测
        //2）整理同属一个文件的活块
        //3）整理活的inode链
        for(int i = 0;i < 3;i++){
            readSegment(segPos + i*SEGMENTLENGTH,segBuf);

            int validLength = *(uint32_t *)segBuf;
            FreeSegment *newf = (FreeSegment *)malloc(sizeof(FreeSegment));
            newf->freeNum = segPos;
            newf->next = freeSeg->next;
            freeSeg->next = newf;

            //开始扫段
            for(int j = 1;j <= validLength;j++ ){
                SSEntry *ss = (SSEntry *)segBuf + j;
                if(ss->blkAttribute == 1 && isAliveFileBlock(segBuf+j*FILEBLOCKSIZE,inodeNum,blk_off)){
                    //是活文件块,直接写入新位置，同时记录对应的块所属inode
                    uint32_t sz = FILEBLOCKSIZE;
                    Inode fake_i;
                    fake_i.inode_num = ss->inode_num;
                    uint32_t writeP = writeOneBlk(&writeSeg,1,&sz,ss->offset,&i,segBuf+j*FILEBLOCKSIZE,0);
                    ListofIndexes *p = getListNode(indexesHeader, &fake_i);
                    
                    if(ss->offset < MAXDIRECT){
                        (p->i).indexes[ss->offset] = writeP;
                        // (p->changedLevel1)[ss->offset] = writeP;
                    } else if(ss->offset < MAXDIRECT + LEVEL2SZ){
                        //二级块索引还没load进来
                        if(!(p->level2indexes)){
                            p->level2indexes = malloc(FILEBLOCKSIZE);
                            Inode i = getInode(inodeNum);
                            readFileBlock(i.indexes[MAXDIRECT],(uint8_t *)(p->level2indexes));
                        }
                        *(p->level2indexes + ss->offset-MAXDIRECT) = writeP;
                    } else{
                        //!三级块逻辑是读入二级块链表然后再更改三级块对应的二级块索引，比较麻烦这里先不写
                    }

                } else if(ss->blkAttribute == 0){
                    //是活inode
                    for(int q = 0;q < ss->offset;q++){
                        Inode *i = (Inode *)(segBuf + j*FILEBLOCKSIZE + q*INODESIZE);
                        if(isAliveInode(i->inode_num,segPos + i*SEGMENTLENGTH + j*FILEBLOCKSIZE/INODESIZE + q)){
                            // ListofIndexes *newN = malloc(ListofIndexes);
                            // newN->i = *i;
                            // newN->next = inodesHeader->next;
                            // inodesHeader->next = newN;
                            getListNode(indexesHeader,i);
                        }
                    }
                } else if(ss->blkAttribute < 0 && isAliveIndexes(segBuf+j*FILEBLOCKSIZE,ss->inode_num,ss->blkAttribute,ss->offset)){
                    //是活高级索引块
                    uint32_t sz = FILEBLOCKSIZE;
                    Inode fake_i;
                    fake_i.inode_num = ss->inode_num;
                    uint32_t writeP = writeOneBlk(&writeSeg,ss->blkAttribute,&sz,ss->offset,&i,segBuf+j*FILEBLOCKSIZE,0);
                    //如果是活的二级块直接索引
                    if(ss->offset == -1){
                        ListofIndexes *p = getListNode(indexesHeader,fake_i);
                        p->i = getInode(ss->inode_num);
                        (p->i).indexes[MAXDIRECT] = writeP;

                        // ListofAliveInode *p = inodesHeader;
                        // while(p->next){
                        //     if((p->next).i.inode_num == ss->inode_num){
                        //         (p->next).i.indexes[MAXDIRECT] = writeP;
                        //         break;
                        //     }
                        // }
                        // if(!(p->next)){
                        //     Inode i = getInode(ss->inode_num);
                        //     i.indexes[MAXDIRECT] = writeP;
                        //     ListofAliveInode *newN = malloc(ListofAliveInode);
                        //     newN->inode = *i;
                        //     newN->next = inodesHeader->next;
                        //     inodesHeader->next = newN;
                        // }

                    } else{
                        //!如果是三级块下的二级块，要更新位置同时更新三级块，比较麻烦这里先不写
                    }
                }
            }
            segPos += SEGMENTLENGTH;
        }
        //将链上的东西，更新到writeSeg中
        //目前的版本还没有考虑三级块的清理问题
        ListofIndexes *p = indexesHeader -> next;
        free(indexesHeader);
        while(p){
            uint32_t sz = FILEBLOCKSIZE;
            if((p->i).level2indexes != NULL) {
                (p->i).indexes[MAXDIRECT] = writeOneBlk(&writeSeg,-2,&sz,-1,&i,(p->i).level2indexes,0);
                free((p->i).level2indexes);
            }
            writeInode(&writeSeg,p->i);
            ListofIndexes *prev = p;
            p = p->next;

            free(prev);
        }
    }
    free(segBuf);
}

//启动的时候先不扫段
FreeSegment* initializeFreeSegment(void){
    FreeSegment *freeHeader = (FreeSegment *)malloc(sizeof(FreeSegment));
    freeHeader ->next = NULL;
    return freeHeader;
}

uint32_t allocateNewSegment(void){
    if(countingSeg() == 0){
        swapSegment(FIRSTSEGSTARTPOS);
    } 
    FreeSegment *p = freeSeg;
    FreeSegment *del = freeSeg->next;
    p->next = del->next;
    uint32_t res = del->freeNum;
    free(del);
    return res;
}