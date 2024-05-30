#ifndef FILEOPS_H
#define FILEOPS_H

#define O_RDONLY        00
#define O_WRONLY        01
#define O_RDWR          02 
#define O_CREAT         00000100
#define O_EXCL          00000200
#define O_TRUNC         00001000
#define O_APPEND        00002000
#define O_SYNC          00010000
// 根据路径串按'/'分隔符，在文件目录树上向下搜寻，拿到对应的inode号检验访问权限是否匹配，
// 如果匹配则返回一个打开文件表项号，否则返回-1
// 实现的flag：
//   1）读写权限：O_RDONLY O_WRONLY O_RDWR
//            对现有文件内容的处理模式 O_APPEND（直接写在后面）、O_TRUNC（丢弃原有内容）、
//            既没有O_APPEND，又没有O_TRUNC，则保持不变
//            O_CREAT 创建一个新文件，如果原文件存在则删除
//            如果和O_EXCL一起使用则如果原文件存在则报错
//   2）同步硬盘：O_SYNC
//            没有这个flag，则只把数据写入缓冲区就返回了，加上这个则写一次就直接放到硬盘上
int open(const char *pathname, uint32_t flags);

ssize_t read(int filedes, void *buf, size_t nbyte);

ssize_t write(int filedes, const void *buf, size_t nbyte);

#endif