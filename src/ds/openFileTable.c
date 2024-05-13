#include "src/ds/openFileTable"
#define O_RDONLY        00
#define O_WRONLY        01
#define O_RDWR          02 
#define O_CREAT         00000100
#define O_EXCL          00000200
#define O_TRUNC         00001000
#define O_APPEND        00002000
#define O_SYNC          00010000

extern OpenFileTable *opentb;

static uint32_t fileDespAllocator(){
    OrderedPairTable used = opentb->used;
    if(used.size == 0) {
        insert(&used,0);
        return 0;
    }
    else if((used.tb)[0].l == 0){
        insert(&used,(used.tb)[0].r+1);
        return (used.tb)[0].r;
    } else{
        insert(&used,(used.tb)[0].l-1);
        return (used.tb)[0].l;
    }
}

OpenFileTable initialize(){
    OpenFileTable opt;
    opt.dummyHead = (OpenEntry *)malloc(sizeof(OpenEntry));
    opt.used = initializeOrderedPairTable();
    return opt;
}

uint32_t putInodeInOpenFileTable(Inode i,uint32_t flags){
    OpenEntry *e = (OpenEntry *)malloc(sizeof(OpenEntry));
    e->filedesp = fileDespAllocator();
    e->offset = 0;
    e->i = i;
    e->permission = flags & 0x03;
    e->next = (opentb->dummyHead).next;
    (opentb->dummyHead).next = e;
    if(flags | O_APPEND){
        e->offset = i.size;
    } 
    return e->filedesp;
}