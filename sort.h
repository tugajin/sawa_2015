#ifndef SORT_H
#define SORT_H

#include "move.h"
#include "misc.h"
#include "generator.h"
#include "boost/utility.hpp"

class Position;

enum Phase { NORMAL_START,//0
             GOOD_CAPTURE_PHASE,//1
             KILLER_PHASE,//2
             QUIET_PHASE1,//3
             QUIET_PHASE2,//4
             BAD_CAPTURE_PHASE,//5
             NORMAL_END,//6
             EVASION_START,//7
             EVASION_PHASE,//8
             EVASION_END,//9
             QUIES_START,//10
             QUIES_CAPTURE_PHASE,//11
             QUIES_END,//12
             PROB_START,//13
             PROB_PHASE,//14
             PROB_END,};//15

enum_op(Phase);

class Sort : boost::noncopyable{
  public:
    Sort(const Position & p, const Depth depth);//full_search
    Sort(const Position & p, const int quies_ply);//quies search
    Sort(const Position & p, const Piece piece, const Depth depth);//prob cut
    template<Color c>Move next_move();
    Move next_move();
    Move killer_[6];//0,1 : killer move
                    //2,3 : counter move
                    //4,5 : follow_up move
    int move_num_;
  private:
    const Position & pos_;
    template<Color c>void next_phase();
    Move trans_move_;
    MoveVal moves_[MAX_MOVE_NUM];
    MoveVal * cur_;
    MoveVal * last_;
    MoveVal * quiet_last_;
    MoveVal * bad_last_;
    Depth depth_;
    int ply_;
    Square recapture_sq_;
    Value threshold_;
    Phase phase_;
};
#endif
