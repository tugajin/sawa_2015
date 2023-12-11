#ifndef GENERATOR_H
#define GENERATOR_H

#include "move.h"
#include "position.h"

class CheckInfo;

enum MoveType { CAPTURE_MOVE, QUIET_MOVE, DROP_MOVE,
                EVASION_MOVE, LEGAL_MOVE, CAPTURE_MOVE2};

struct MoveVal {
  Move move;
  Value value;
};

template<Color c, MoveType mt> MoveVal * gen_move(const Position & pos, MoveVal *mlist);
template<MoveType mt> extern MoveVal * gen_move(const Position & pos, MoveVal * mlist);
template<Color c, MoveType mt>extern MoveVal * gen_move(const Position & pos, MoveVal * mlist);

template<Color c,bool mate_three_flag>extern MoveVal * gen_check(const Position & pos, MoveVal * mlist, const CheckInfo * ci);
static inline MoveVal * gen_check(const Position & pos, MoveVal * mlist, const CheckInfo * ci){
  return (pos.turn()) ? gen_check<WHITE,false>(pos,mlist,ci) : gen_check<BLACK,false>(pos,mlist,ci);
}
#endif
