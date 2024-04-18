#include "src/ds/openFileTable"
#define O_RDONLY        00
#define O_WRONLY        01
#define O_RDWR          02 
#define O_CREAT         00000100
#define O_EXCL          00000200
#define O_TRUNC         00001000
#define O_APPEND        00002000
#define O_SYNC          00010000

uint32_t putInodeInOpenFileTable(OpenFileTable tb,Inode i,uint32_t flags){
    OpenEntry *e = (OpenEntry *)malloc(sizeof(OpenEntry));
    e->permission = flags & 0x03;
    if(flags & )
}