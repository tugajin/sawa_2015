#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "type.h"
#include "misc.h"
#include "piece.h"

class Position;

enum EvalIndex{
  f_hand_pawn   =    0,
  e_hand_pawn   =   19,
  f_hand_lance  =   38,
  e_hand_lance  =   43,
  f_hand_knight =   48,
  e_hand_knight =   53,
  f_hand_silver =   58,
  e_hand_silver =   63,
  f_hand_gold   =   68,
  e_hand_gold   =   73,
  f_hand_bishop =   78,
  e_hand_bishop =   81,
  f_hand_rook   =   84,
  e_hand_rook   =   87,
  fe_hand_end   =   90,
  f_pawn        =   81,
  e_pawn        =  162,
  f_lance       =  225,
  e_lance       =  306,
  f_knight      =  360,
  e_knight      =  441,
  f_silver      =  504,
  e_silver      =  585,
  f_gold        =  666,
  e_gold        =  747,
  f_bishop      =  828,
  e_bishop      =  909,
  f_horse       =  990,
  e_horse       = 1071,
  f_rook        = 1152,
  e_rook        = 1233,
  f_dragon      = 1314,
  e_dragon      = 1395,
  fe_end        = 1476,
  pos_n         = fe_end * (fe_end+1)/2,
  kkp_hand_pawn   =   0,
  kkp_hand_lance  =  19,
  kkp_hand_knight =  24,
  kkp_hand_silver =  29,
  kkp_hand_gold   =  34,
  kkp_hand_bishop =  39,
  kkp_hand_rook   =  42,
  kkp_hand_end    =  45,
  kkp_pawn        =  36,
  kkp_lance       = 108,
  kkp_knight      = 171,
  kkp_silver      = 252,
  kkp_gold        = 333,
  kkp_bishop      = 414,
  kkp_horse       = 495,
  kkp_rook        = 576,
  kkp_dragon      = 657,
  kkp_end         = 738,
  };

enum_op(EvalIndex);

const EvalIndex kkp_index[PIECE_TYPE_SIZE] = {
  EvalIndex(-100000),
  kkp_gold, kkp_gold, kkp_gold, kkp_gold, kkp_horse, kkp_dragon,
  EvalIndex(-100000), kkp_gold,
  kkp_pawn, kkp_lance, kkp_knight, kkp_silver, kkp_bishop, kkp_rook,
};
const EvalIndex pc_on_sq_index[PIECE_SIZE] = {
  EvalIndex(-100000),
  f_gold, f_gold, f_gold, f_gold, f_horse, f_dragon,
  EvalIndex(-100000), f_gold,
  f_pawn, f_lance, f_knight, f_silver, f_bishop, f_rook,
  EvalIndex(-100000), EvalIndex(-100000),
  e_gold, e_gold, e_gold, e_gold, e_horse, e_dragon,
  EvalIndex(-100000), e_gold,
  e_pawn, e_lance, e_knight, e_silver, e_bishop, e_rook,
  EvalIndex(-100000),
};

extern Value p_value[PIECE_SIZE];
extern Value p_value_ex[PIECE_SIZE];
extern Value p_value_pm[PIECE_SIZE];
extern Kvalue pc_on_sq[SQUARE_SIZE][fe_end][fe_end];
extern Kvalue akkp[SQUARE_SIZE][SQUARE_SIZE][kkp_end];

extern Value evaluate_material(const Position & pos);
extern Value evaluate(Position & pos);
template<Color c>extern Value evaluate(Position & pos);
extern void init_evauator();
extern void set_piece_value();
extern void init_eval_trans();
extern void clear_weight();

void print_material();

static inline Value piece_value(const Piece p){
  ASSERT(p == EMPTY || piece_is_ok(p));
  return p_value[p];
}
static inline Value piece_type_value(const PieceType pt){
  ASSERT( pt == EMPTY_T || piece_type_is_ok(pt));
  return p_value[pt];
}
static inline Value piece_value_ex(const Piece p){
  ASSERT(p == EMPTY || piece_is_ok(p));
  return p_value_ex[p];
}
static inline Value piece_type_value_ex(const PieceType pt){
  ASSERT( pt == EMPTY_T || piece_type_is_ok(pt));
  return p_value_ex[pt];
}
static inline Value piece_value_pm(const Piece p){
  ASSERT(p == EMPTY || piece_is_ok(p));
  return p_value_pm[p];
}
static inline Value piece_type_value_pm(const PieceType pt){
  ASSERT( pt == EMPTY_T || piece_type_is_ok(pt));
  return p_value_pm[pt];
}
static inline Value pc_pc_on_sq(const Square k,
                                const EvalIndex k0,
                                const EvalIndex l0){
  ASSERT(square_is_ok(k));
  ASSERT(int(k0) >= 0);
  ASSERT(k0 < fe_end);
  ASSERT(int(l0) >= 0);
  ASSERT(l0 < fe_end);
  return Value(pc_on_sq[k][k0][l0]);
}
static inline Value kkp(const Square bk,
                        const Square wk,
                        const EvalIndex p ){
  ASSERT(square_is_ok(bk));
  ASSERT(square_is_ok(wk));
  ASSERT(int(p) >= 0);
  ASSERT(p < kkp_end);
  return Value(akkp[bk][wk][p]);
}
const Value FV_SCALE = Value(32);

#endif
