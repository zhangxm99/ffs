#include "orderedPairTable.h"
#include <stdlib.h>

OrderedPairTable initializeOrderedPairTable(void) {
    OrderedPairTable opt;
    opt.tb = (pair *)malloc(sizeof(pair) * 10);  // 初始大小设为10
    if (opt.tb == NULL) {
        exit(1);  // 内存分配失败
    }
    opt.room = 10;
    opt.size = 0;
    return opt;
}

void insert(OrderedPairTable *opt, uint32_t val) {
    if (opt->size == 0) {
        opt->tb[0].l = val;
        opt->tb[0].r = val;
        opt->size = 1;
        return;
    }

    // 扩展数组空间
    if (opt->size == opt->room) {
        opt->room *= 2;
        opt->tb = (pair *)realloc(opt->tb, sizeof(pair) * opt->room);
        if (opt->tb == NULL) {
            exit(1);  // 内存分配失败
        }
    }

    int low = 0, high = opt->size - 1, mid;
    while (low <= high) {
        mid = low + (high - low) / 2;
        if (val < opt->tb[mid].l - 1) {
            high = mid - 1;
        } else if (val > opt->tb[mid].r + 1) {
            low = mid + 1;
        } else {
            // 合并区间或插入点
            if (val == opt->tb[mid].l - 1) {
                opt->tb[mid].l = val;
            }
            if (val == opt->tb[mid].r + 1) {
                opt->tb[mid].r = val;
            }
            return;
        }
    }

    // 插入新段
    for (int j = opt->size; j > low; j--) {
        opt->tb[j] = opt->tb[j - 1];
    }
    opt->tb[low].l = val;
    opt->tb[low].r = val;
    opt->size++;
}

int32_t isIn(OrderedPairTable opt, uint32_t val) {
    int low = 0, high = opt.size - 1, mid;
    while (low <= high) {
        mid = low + (high - low) / 2;
        if (val < opt.tb[mid].l) {
            high = mid - 1;
        } else if (val > opt.tb[mid].r) {
            low = mid + 1;
        } else {
            return 1;  // 找到值
        }
    }
    return 0;  // 未找到值
}

void remove(OrderedPairTable *opt, uint32_t val) {
    int low = 0, high = opt->size - 1, mid;
    while (low <= high) {
        mid = low + (high - low) / 2;
        if (val < opt->tb[mid].l) {
            high = mid - 1;
        } else if (val > opt->tb[mid].r) {
            low = mid + 1;
        } else {
            if (val == opt->tb[mid].l) {
                if (opt->tb[mid].l == opt->tb[mid].r) {
                    // 删除整个段
                    for (int j = mid; j < opt->size - 1; j++) {
                        opt->tb[j] = opt->tb[j + 1];
                    }
                    opt->size--;
                } else {
                    opt->tb[mid].l++;
                }
            } else if (val == opt->tb[mid].r) {
                opt->tb[mid].r--;
            } else {
                // 分裂段
                pair newPair = {val + 1, opt->tb[mid].r};
                opt->tb[mid].r = val - 1;
                insert(opt, newPair.l);
            }
            return;
        }
    }
}