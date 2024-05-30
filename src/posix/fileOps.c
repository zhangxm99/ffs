#include "src/ds/dentryTree.h"
#include "src/ds/inodeMap.h"
#include "fileOps.h"
#include "src/ds/inode.h"
#include <stdio.h>

extern DentryTree *root;
extern InodeMap *imap;
extern FlashDev dev;
extern OpenFileTable tb;

int open(const char *pathname, uint32_t flags){
    int inode_num = searchInode(path,flags&0x03,flags & O_CREAT,flags & O_EXCL,flags &O_TRUNC);
    if(inode < 0){
        if(inode == -1) printf("privilege error\n");
        else if(inode == -2) printf("file not exists or path error\n");
        else printf("file already exists\n");
        return -1;
    }
    Inode i = getInode(inode_num);
    uint32_t filedesp = putInodeInOpenFileTable(i,flags);
    if(filedesp == -1){
        printf("Opened too much\n");
        return -1;
    }
    return filedesp;

}

ssize_t read(int filedes, void *buf, size_t nbyte) {
    OpenEntry *entry = findOpenEntry(filedes);  // 查找文件描述符对应的打开文件条目
    if (entry == NULL) {
        return -1;  // 文件描述符无效
    }

    size_t bytes_read = 0;
    uint8_t *buffer = (uint8_t *)buf;

    while (bytes_read < nbyte) {
        uint32_t block_num = entry->offset / FILEBLOCKSIZE;
        uint32_t block_offset = entry->offset % FILEBLOCKSIZE;
        uint32_t bytes_to_read = FILEBLOCKSIZE - block_offset;

        if (bytes_to_read > nbyte - bytes_read) {
            bytes_to_read = nbyte - bytes_read;
        }

        uint8_t *block_data = findBlockInReadBuffer(entry->i.inode_num, block_num);
        if (block_data == NULL) {
            readBlockToBuffer(block_num, block_offset);  // 从磁盘读取块到缓存
            block_data = findBlockInReadBuffer(entry->i.inode_num, block_num);
            if (block_data == NULL) {
                return -1;  // 读取失败
            }
        }

        memcpy(buffer + bytes_read, block_data + block_offset, bytes_to_read);
        bytes_read += bytes_to_read;
        entry->offset += bytes_to_read;
    }

    return bytes_read;
}

ssize_t write(int filedes, const void *buf, size_t nbyte) {
    OpenEntry *entry = findOpenEntry(filedes);  // 查找文件描述符对应的打开文件条目
    if (entry == NULL) {
        return -1;  // 文件描述符无效
    }

    if ((entry->permission & O_WRONLY) == 0 && (entry->permission & O_RDWR) == 0) {
        return -1;  // 检查是否有写权限
    }

    uint32_t bytes_written = 0;
    const uint8_t *buffer = (const uint8_t *)buf;

    while (bytes_written < nbyte) {
        uint32_t block_num = entry->offset / FILEBLOCKSIZE;
        uint32_t block_offset = entry->offset % FILEBLOCKSIZE;
        uint32_t bytes_to_write = FILEBLOCKSIZE - block_offset;

        if (bytes_to_write > nbyte - bytes_written) {
            bytes_to_write = nbyte - bytes_written;
        }

        // 调用writeToBuf来处理写入逻辑
        int32_t result = writeToBuf(entry->i, entry->offset, (uint8_t *)(buffer + bytes_written), bytes_to_write);
        if (result == -1) {
            return -1;  // 写入失败
        }

        bytes_written += bytes_to_write;
        entry->offset += bytes_to_write;
    }

    return bytes_written;
}
