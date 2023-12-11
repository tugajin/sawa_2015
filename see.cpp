#include "type.h"
#include "position.h"
#include "move.h"
#include <limits.h>

#ifndef NDEBUG
static u32 debug_flag = 0;
#define DOUT(...) if(debug_flag){ printf("df = %d ",debug_flag);\
                                  printf(__VA_ARGS__); }
#define DOUT2(...) if(debug_flag){ printf(__VA_ARGS__); }
#else
#define DOUT(...)
#define DOUT2(...)
#endif

#define CAN_PROM_PIECE_FOO(PIECE) do{bb = bb_attack & piece_bb(opp_color(c),PIECE);\
                                   if(bb.is_not_zero()){\
                                       from = bb.lsb<N>();\
                                       attacked_piece = piece_type_value_ex((PIECE));\
                                       if(is_prom_rank(square_to_rank(to),opp_color(c))){\
                                         value += piece_type_value_pm((PIECE));\
                                         attacked_piece += piece_type_value_pm((PIECE));\
                                       }\
                                       goto ab_cut;\
                                     }}while(0)

#define CAN_PROM_PIECE_FOO2(PIECE) do{bb = bb_attack & piece_bb(opp_color(c),PIECE);\
                                   if(bb.is_not_zero()){\
                                       from = bb.lsb<N>();\
                                       attacked_piece = piece_type_value_ex((PIECE));\
                                       if(is_prom_rank(square_to_rank(to),opp_color(c))\
                                       || is_prom_rank(square_to_rank(from),opp_color(c))){\
                                         value += piece_type_value_pm((PIECE));\
                                         attacked_piece += piece_type_value_pm((PIECE));\
                                       }\
                                       goto ab_cut;\
                                     }}while(0)

#define NOT_PROM_PIECE_FOO(PIECE) do{bb = bb_attack & piece_bb(opp_color(c),PIECE);\
                                  if(bb.is_not_zero()){\
                                      from = bb.lsb<N>();\
                                      attacked_piece = piece_type_value_ex((PIECE));\
                                      goto ab_cut;\
                                   }}while(0)

Value Position::see(const Move & move, const Value root_alpha, const Value root_beta)const{

  ASSERT(move.is_ok());
  ASSERT(is_ok(move));
  ASSERT(root_alpha < root_beta);
  DOUT("start see\n");
  auto from = move.from();
  auto to = move.to();
  auto alpha = Value(INT_MIN) + 2;
  auto beta = VALUE_ZERO;
  auto c = turn();
  int nc;
  PieceType piece_t;
  PieceType capture_t;
  Bitboard bb;
  Value attacked_piece, value, xvalue;

  if(square_is_drop(from)){
    value = VALUE_ZERO;
    const auto hp = move.hand_piece();
    piece_t = hand_piece_to_piece_type(hp);
    from = to;//hack to skip square relation
    attacked_piece = piece_type_value_ex(piece_t);
  }else{
    piece_t = piece_to_piece_type(square(from));
    capture_t = piece_to_piece_type(square(to));
    value = piece_type_value_ex(capture_t);
    attacked_piece = piece_type_value_ex(piece_t);
    if(move.is_prom()){
      value += piece_type_value_pm(piece_t);
      attacked_piece += piece_type_value_pm(piece_t);
    }
    xvalue = value - attacked_piece - piece_type_value_pm(PPAWN);
    DOUT("check futility cut xvalue %d >= %d ? \n",xvalue,root_beta);
    if(xvalue >= root_beta){
      DOUT("root futility cut \n");
      return xvalue;
    }
  }
  Bitboard occ_bb = this->occ_bb();
  Bitboard bb_attack = attack_to(to,occ_bb);
    // king capture pruning
    //New Architectures in Computer Chess
    //Fritz Reul
  //if(piece_t == KING && (bb_attack & color_bb(opp_color(c))).is_not_zero()){
  //  DOUT("king cut in root\n");
   // return alpha;
  //}
  Bitboard bb_attack_bishop = piece_bb(BISHOP,PBISHOP);
  Bitboard bb_attack_rook = piece_bb(ROOK,PROOK);
  bb_attack_rook |= piece_bb(LANCE_B) & get_pseudo_attack<LANCE>(WHITE,to);
  bb_attack_rook |= piece_bb(LANCE_W) & get_pseudo_attack<LANCE>(BLACK,to);
  beta = value;

  for(nc = 1;;++nc){
    DOUT("turn %d\n",c);
    DOUT("alpha %d value %d beta %d\n",alpha,value,beta);
    DOUT("from %d to %d %s\n",from,to,str_piece_name(square(from)).c_str());
    ASSERT(nc < 100);
    occ_bb.Xor(from);
    DOUT("update attacker\n");
    //remove an attacker, and add a hidden piece to bb_attack
    switch(get_square_relation(from,to)){
      case RANK_RELATION : case FILE_RELATION :
        DOUT("update rook\n");
        bb_attack |= get_rook_attack(to,occ_bb)
                      & bb_attack_rook;
        break;
      case LEFT_UP_RELATION : case RIGHT_UP_RELATION :
        DOUT("update bishop\n");
        bb_attack |= get_bishop_attack(to,occ_bb)
                      & bb_attack_bishop;
        break;
      default: break;
    }
    bb_attack &= occ_bb;
    DOUT("after update attacker \n");
    //find the cheapest piece attacks the target
    if(!(bb_attack & color_bb(opp_color(c))).is_not_zero()){
      DOUT("can move piece is nothing\n");
      break;
    }
    value = attacked_piece - value;

    CAN_PROM_PIECE_FOO(PAWN);
    CAN_PROM_PIECE_FOO(LANCE);
    CAN_PROM_PIECE_FOO(KNIGHT);
    NOT_PROM_PIECE_FOO(PPAWN);
    NOT_PROM_PIECE_FOO(PLANCE);
    NOT_PROM_PIECE_FOO(PKNIGHT);
    CAN_PROM_PIECE_FOO2(SILVER);
    NOT_PROM_PIECE_FOO(PSILVER);
    NOT_PROM_PIECE_FOO(GOLD);
    CAN_PROM_PIECE_FOO2(BISHOP);
    NOT_PROM_PIECE_FOO(PBISHOP);
    CAN_PROM_PIECE_FOO2(ROOK);
    NOT_PROM_PIECE_FOO(PROOK);
    bb = bb_attack & piece_bb(opp_color(c),KING);

    if(bb.is_not_zero()){
      from = bb.lsb<N>();
      attacked_piece = piece_type_value_ex(KING);
    }
    DOUT("king passed\n");
    // king capture pruning
    //New Architectures in Computer Chess
    //Fritz Reul
    //if((bb_attack & color_bb(c)).is_not_zero()){
    //  DOUT("king cut\n");
    //  break;
    //}
    ab_cut:
    c = opp_color(c);
    if(nc & 1){
      if(-value > alpha){
        if(-value >= beta){
          DOUT("beta cut\n");
          return beta;
        }
        if(-value >= root_beta){
          DOUT("root_beta cut\n");
          return -value;
        }
        alpha = -value;
      }else{
        xvalue = attacked_piece + piece_type_value_pm(PPAWN)- value;
        if(xvalue <= alpha){
          DOUT("alpha cut\n");
          return alpha;
        }
        if(xvalue <= root_alpha){
          DOUT("root_alpha cut\n");
          return xvalue;
        }
      }
    }else{
      if(value < beta){
        if(value <= alpha){
          DOUT("alpha cut2\n");
          return alpha;
        }
        if(value <= root_alpha){
          DOUT("root_alpha cut2\n");
          return value;
        }
        beta = value;
      }else{
        xvalue = value - attacked_piece - piece_type_value_pm(PPAWN);
        if(xvalue >= beta){
          DOUT("beta cut2\n");
          return beta;
        }
        if(xvalue >= root_beta){
          DOUT("root_beta cut2\n");
          return xvalue;
        }
      }
    }
  }
  if(nc & 1){
    return beta;
  }else{
    return alpha;
  }
}

#undef CAN_PROM_PIECE_FOO
#undef CAN_PROM_PIECE_FOO2
#undef NOT_PROM_PIECE_FOO
