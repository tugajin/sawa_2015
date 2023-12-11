#ifndef RANDOM_H
#define RANDOM_H

#include "type.h"

extern u64 random64();
extern u32 random32();
extern int random32(const int v);
extern u64 random_for_magic();
extern void init_rand();

#endif
