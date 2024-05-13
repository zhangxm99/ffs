#include <stdint.h>
#include "inodeMap.h"

extern InodeMap *imap;

static inline uint32_t hash32shift(uint32_t key) 
{ 
    key = ~key + (key << 15);
    key = key ^ (key >> 12); 
    key = key + (key << 2); 
    key = key ^ (key >> 4); 
    key = key * 2057;
    key = key ^ (key >> 16); 
    return key % MAXINDEX; 
}

InodeMap generateInodeMap(){
    InodeMap imap;
    for(int i = 0;i < MAXINDEX;i++){
        imap.mp[i] = (ListofPhy *)malloc(sizeof(ListofPhy));
    }

    uint8_t buf[FILEBLOCKSIZE];
    if(readFileBlock(0,buf) != 0){
        return NULL;
    }
    CheckPoint* cr = (CheckPoint*)buf;
    uint32_t imapPos = cr->imapPos;
    uint32_t imapSz  = cr->imapSize;
    imap.size = imapSz;

    for(int i = 0,sz = imapSz;i < ((imapSz-1)/FILEBLOCKSIZE + 1);i++,sz-=FILEBLOCKSIZE){
        if(readFileBlock(imapPos + i,buf) != 0){
            return NULL;
        }
        for(int j = 0;j < min(sz,FILEBLOCKSIZE);j+=2*sizeof(uint32_t)){
            insertInode(imap.mp,*(uint32_t *)(buf + j),*(uint32_t *)(buf + j + sizeof(uint32_t)));
        }
    }
    return imap;
}

uint32_t* mapInode(uint32_t inode_num){
    int offset = hash32shift(inode_num);
    ListofPhy *p = imap->mp[offset];
    while(p){
        if(p->key == inode_num) return &(p->value);
        p = p->next;
    }
    return NULL;
}

void deleteInode(uint32_t inode_num){
    int offset = hash32shift(inode_num);
    ListofPhy *p = imap->mp[offset];
    while(p->next){
        if((p->next).key == inode_num) {
            ListofPhy *n = (p->next).next;
            free(p->next);
            p->next = n;
        }
        p = p->next;
    }
}

void insertInode(uint32_t inode_num,uint32_t phy){
    int offset = hash32shift(inode_num);
    ListofPhy *p = imap->mp[offset];

    ListofPhy *n = (ListofPhy *)malloc(sizeof(ListofPhy));
    *n = (ListofPhy){inode_num,phy,NULL};
    n->next = p->next;
    p->next = n;
}