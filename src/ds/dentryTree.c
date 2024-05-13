#include "src/ds/dentryTree.h"
#include "src/ds/inode.h"
#include <string.h>

extern DentryTree *treeRoot;

DentryTree generateTree(){
    return dfs(0);
}

inline Inode getInode(uint32_t inode_num){
    uint32_t p = *mapInode(inode_num);
    uint32_t blk = p / INODESIZE, offset = p % INODESIZE;
    uint8_t buf[FILEBLOCKSIZE];
    if(readFileBlock(blk,buf) != 0){
        return NULL;
    }
    Inode inode = *(Inode *)(buf + offset);
    return inode;
}

//给一个一级块和树结点列表，继续串起来
static ListofTreeNode* l1parseAndConnect(uint8_t *buf,uint32_t sz,ListofTreeNode *p){
    for(int j = 0;j < sz/sizeof(uint32_t);j+=4){
        p->next = (ListofTreeNode*) malloc(sizeof(ListofTreeNode));
        p = p->next;
        p->next = NULL;
        DentryTree new = dfs(*(uint32_t *)(buf + j));
        p->node = new;
    }
    return p;
}

//给一个二级块和树结点，继续串起来
static ListofTreeNode* l2parseAndConnect(uint8_t *buf1,uint32_t *sz,ListofTreeNode *p){
    uint8_t buf[FILEBLOCKSIZE];
    uint32_t j = 0;
    while(*sz > 0){
        if(readFileBlock(*(uint32_t*)(buf1 + j),buf) != 0){
            return NULL;
        }
        p = l1parseAndConnect(buf,min(*sz,FILEBLOCKSIZE),p);
        *sz -= FILEBLOCKSIZE;
        j += sizeof(uint32_t);
    }
    return p;
}

static DentryTree dfs(uint32_t inode_num){
    Inode root = getInode(inode_num);
    DentryTree n = (DentryTree){root->isDirAndPermission,"",inode_num,NULL};
    memcpy((void*)&(n.filename),(void*)&(root->name),MAXSTRINGLENGTH);

    if(n->isDir){
        return n;
    }

    uint32_t sz = inode -> size;
    ListofTreeNode *dummyHead = (ListofTreeNode*) malloc(sizeof(ListofTreeNode));
    ListofTreeNode *p = dummyHead;

    uint8_t buf[FILEBLOCKSIZE];

    //先看一级块
    for(uint32_t i = 0;i < MAXDIRECT;i++){
        if(readFileBlock((inode->indexes)[i],buf) != 0){
            return NULL;
        }
        p = l1parseAndConnect(buf,min(sz,FILEBLOCKSIZE),p);
        sz -= FILEBLOCKSIZE;
    }

    //再看是否需要访问二级块
    uint8_t buf1[FILEBLOCKSIZE];
    if(sz > 0){
        if(readFileBlock((inode->indexes)[MAXDIRECT],buf1) != 0){
            return NULL;
        }
        p = l2parseAndConnect(buf1,&sz,p);
    }

    //再看是否需要访问三级块
    if(sz > 0){
        if(readFileBlock((inode->indexes)[MAXDIRECT+1],buf1) != 0){
            return NULL;
        }
        uint32_t i = 0;
        while(sz > 0){
            if(readFileBlock(*(uint32_t*)(buf1+j),buf) != 0){
                return NULL;
            }
            p = l2parseAndConnect(buf,&sz,p);
            i += sizeof(uint32_t);
        }
    }

    n->nextLevel = dummyHead;
    return n;
}

int32_t searchInode(const char *path,uint8_t permission,uint32_t isCreat,uint32_t is_excl,uint32_t is_trunc){
    DentryTree *prev;
    DentryTree *now = treeRoot;
    char seg[MAXSTRINGLENGTH];
    for(int i = 0,j = 0;;i++){
        if(path[i] == '/' || path[i] == '\0'){
            seg[j] = '\0';
            //比较匹配的项
            const ListofTreeNode *p = now->nextLevel;
            while(p->next){
                if(strcmp(((p->next).node).filename,seg) == 0){
                    prev = now;
                    now = &((p->next).node);
                    break;
                } 
                p = p->next;
            }
            if(!(p->next)) {
                //如果这是路径的最后一项且要创建
                if(isCreat && path[i] == '\0'){
                    //分别在树上和硬盘上创建inode
                    Inode newi;
                    newi.inode_num = inodeNumAllocator();
                    strcpy(newi.name,seg);
                    newi.isDirAndPermission = 0x8180;
                    newi.ownerID = 0;
                    newi.size = 0;

                    //挂一个新的inode在树上，再写入硬盘
                    p->next = (ListofTreeNode*)malloc(sizeof(ListofTreeNode));
                    (p->next).next = NULL;
                    (p->next).node.isDirAndPermission = 0x8180;
                    strcpy((p->next).node.filename,seg);
                    (p->next).node.inode_num = newi.inode_num;

                    uint32_t pos = writeToBuf(newi,0,NULL,0);

                    Inode dirI = getInode(prev.inode_num);
                    writeToBuf(dirI,dirI.size,&pos,4);
                    
                    return now->inode_num;
                }
                return -1;
            }
            if(path[i] == '\0') break;
            j = 0;
        } else{
            seg[j] = path[i];
            j++;
        }
    }
    //如果最后不是一个文件则路径不合法
    if(now->isDirAndPermission&0x8000){
        return -2;
    } 
    //文件没有写权限要写
    //文件没有读权限要读
    //上层目录没有写权限要写当前文件
    //这三种情况都是不合法的
    if((!(now->isDirAndPermission & 0x100) && permission == 0) || 
    (!(now->isDirAndPermission & 0x80) && permission != 0) || 
    (!(prev->isDirAndPermission & 0x80) && permission != 0)){
        return -1;
    }
    //如果文件存在且有CREAT
    if(isCreat){
        if(is_excl) return -3;
    } 
    //如果文件存在且有TRUNC,则修改inode将size打成0
    if(is_trunc){
        Inode i = getInode(now->inode_num);
        i.size = 0;
        writeToBuf(i,0,NULL,0);
    }
    return now->inode_num;

}