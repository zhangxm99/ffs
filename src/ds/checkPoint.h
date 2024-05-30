#ifndef CHECKPOINT_H
#define CHECKPOINT_H
#include <stdint.h>

typedef struct{
    uint32_t timestamp;
    uint32_t imapPos;
    uint32_t imapSize;
} CheckPoint;

#endif