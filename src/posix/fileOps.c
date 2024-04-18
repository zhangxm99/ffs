#include "src/ds/dentryTree.h"
#include "src/ds/inodeMap.h"
#include "fileOps.h"
#include "src/ds/inode.h"
#include <stdio.h>

extern DentryTree root;
extern InodeMap imap;
extern FlashDev dev;
extern OpenFileTable tb;

int open(const char *pathname, uint32_t flags){
    int inode_num = searchInode(root,path,flags&0x03,flags & O_CREAT,flags & O_EXCL);
    if(inode < 0){
        if(inode == -1) printf("privilege error\n");
        else if(inode == -2) printf("file not exists or path error\n");
        else printf("file already exists\n");
        return -1;
    }
    Inode i = getInode(dev,&imap,inode_num);
    uint32_t filedesp = putInodeInOpenFileTable(tb,i,flags);
    if(filedesp == -1){
        printf("Opened too much\n");
        return -1;
    }
    return filedesp;

}

