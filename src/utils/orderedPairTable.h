#ifndef ORDEREDPAIRTABLE_H
#define ORDEREDPAIRTABLE_H

//该文件实现了一个有序段表数据结构，为各种序号的分配与管理提供服务
//其中表中每一个元素是一个pair，表示连续的序号段
typedef struct{
    uint32_t l;
    uint32_t r;
} pair;

typedef struct{
    //放在堆上
    pair *tb;
    //拿到分配的空间大小，不是实际存储的大小
    uint32_t room;
    //存储的实际元素大小
    uint32_t size;
} OrderedPairTable;


//初始化
OrderedPairTable initializeOrderedPairTable(void);

//向表中插入一个元素，使用二分查找定位
//前者需要分裂段，后者需要独立成段（即（val,val）），注意如果超过了分配的空间则需要重新复制翻倍
void insert(OrderedPairTable *,uint32_t val);

//二分查找看是否val在段内
int32_t isIn(OrderedPairTable,uint32_t val);

//删除val，两种情况，一是段端点，则修改端点值（注意如果独立成段则直接删除整段），二是段内值，则分裂段
void remove(OrderedPairTable *,uint32_t val);

#endif