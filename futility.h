#ifndef FUTILITY_H

#include "type.h"
#include <algorithm>

extern Value futility_move_count[2][32];
extern Depth reduction[2][2][64][64];
extern void init_futility();

static inline Value get_futility_margin(const Depth d){
    return Value(100 * int(d));
}
static inline Value get_move_count(const bool improving, const Depth d){
  ASSERT(int(d) >= 0);
  ASSERT(int(d) <= 31);
  return futility_move_count[improving][d];
}
static inline Depth get_reduction(const bool pv, const bool improve,const Depth d, const int num){
  ASSERT(int(d) >= 0);
  ASSERT(num >= 0);
  return reduction[pv][improve][std::min(int(d/DEPTH_ONE),63)][std::min(num,63)];
}

#endif
