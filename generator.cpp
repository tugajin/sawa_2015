#include "type.h"
#include "piece.h"
#include "move.h"
#include "position.h"
#include "generator.h"
#include "misc.h"
#include "bitboard.h"
#include "boost/static_assert.hpp"
#include "boost/utility.hpp"

// noprom move ppawn,plance,pknight,psilver,pbishop,prook,gold
template <Color c, PieceType pt, MoveType mt>
inline MoveVal * gen_no_prom_move(const Position & pos, MoveVal * mlist, const Bitboard & target){

  ASSERT(!can_prom_piece_type(pt));
  ASSERT(pt != KING);
  ASSERT(mlist != nullptr);
  ASSERT(pos.is_ok());

  Bitboard piece = (pt == GOLDS) ? pos.golds_bb(c)
    : pos.piece_bb(c,pt);
  while(piece.is_not_zero()){
    const Square from = piece.lsb<D>();
    Bitboard att = pos.attack_from<c,pt>(from) & target;
    while(att.is_not_zero()){
      (mlist++)->move = make_move(from,att.lsb<D>());
    }
  }
  return mlist;
}
//king
template<Color c, MoveType mt>
inline MoveVal * gen_king_move(const Position & pos, MoveVal * mlist, const Bitboard & target){

  ASSERT(mlist != nullptr);
  ASSERT(pos.is_ok());

  const Square from = pos.king_sq(c);
  Bitboard att = pos.attack_from<c,KING>(from) & target;
  while(att.is_not_zero()){
    (mlist++)->move = make_move(from,att.lsb<D>());
  }
  return mlist;
}
//pawn
template<Color c, MoveType mt> inline MoveVal * gen_pawn_move
(const Position & pos, MoveVal * mlist, const Bitboard & target){

  ASSERT(mlist != nullptr);
  ASSERT(pos.is_ok());
  const Bitboard piece = pos.piece_bb(c,PAWN);
  Bitboard att = get_pawn_attack<c>(piece) & target;
  Bitboard tmp_att = att & get_prom_area(c);
  Square to;
  if(mt != QUIET_MOVE){
    //prom
    BB_FOR_EACH(tmp_att,to,
        const Square from = (c == BLACK) ? (to + DOWN) : (to + UP);
        (mlist++)->move = make_prom_move(from,to);
        );
  }
  //no prom
  tmp_att = att & get_not_prom_area(c);
  BB_FOR_EACH(tmp_att,to,
      const Square from = (c == BLACK) ? (to + DOWN) : (to + UP);
      (mlist++)->move = make_move(from,to);
      );
  return mlist;
}
// bishop,rook
template <Color c, PieceType pt, MoveType mt> inline MoveVal * gen_bishop_rook_move
(const Position & pos, MoveVal * mlist, const Bitboard & target){

  ASSERT(mlist != nullptr);
  ASSERT(pos.is_ok());
  Bitboard piece = pos.piece_bb(c,pt);
  Bitboard tmp_piece = piece & get_prom_area(c);
  piece &= get_not_prom_area(c);
  const Bitboard target_1to3 = (mt == CAPTURE_MOVE) ? (~pos.color_bb(c)) : target;
  const Bitboard target_4to9 = target;
  if(mt != QUIET_MOVE){
    //1-3
    while(tmp_piece.is_not_zero()){
      const Square from = tmp_piece.lsb<D>();
      Bitboard att = pos.attack_from<c,pt>(from) & target_1to3;
      while(att.is_not_zero()){
        (mlist++)->move = make_prom_move(from,att.lsb<D>());
      }
    }
  }
  //4-9
  while(piece.is_not_zero()){
    const Square from = piece.lsb<D>();
    Bitboard att = pos.attack_from<c,pt>(from) & target_4to9;
    Bitboard tmp_att = att & get_prom_area(c);
    att &= get_not_prom_area(c);
    if(mt != QUIET_MOVE){
      //prom
      while(tmp_att.is_not_zero()){
        (mlist++)->move = make_prom_move(from,tmp_att.lsb<D>());
      }
    }
    //no prom
    while(att.is_not_zero()){
      (mlist++)->move = make_move(from,att.lsb<D>());
    }
  }
  return mlist;
}
//silver
template <Color c, MoveType mt> inline MoveVal * gen_silver_move
(const Position & pos, MoveVal * mlist, const Bitboard & target){

  ASSERT(mlist != nullptr);
  ASSERT(pos.is_ok());
  Bitboard piece = pos.piece_bb(c,SILVER);
  Bitboard tmp_piece = piece & get_prom_area(c);
  //1-3
  while(tmp_piece.is_not_zero()){
    const Square from = tmp_piece.lsb<D>();
    Bitboard att = pos.attack_from<c,SILVER>(from) & target;
    while(att.is_not_zero()){
      const Square to = att.lsb<D>();
      (mlist++)->move = make_prom_move(from,to);
      (mlist++)->move = make_move(from,to);
    }
  }
  //4
  tmp_piece = piece & get_pre_prom_area(c);
  while(tmp_piece.is_not_zero()){
    const Square from = tmp_piece.lsb<D>();
    Bitboard att = pos.attack_from<c,SILVER>(from) & target;
    Bitboard tmp_att = att & get_prom_area(c);
    att &= get_not_prom_area(c);
    while(tmp_att.is_not_zero()){
      const Square to = tmp_att.lsb<D>();
      (mlist++)->move = make_prom_move(from,to);
      (mlist++)->move = make_move(from,to);
    }
    while(att.is_not_zero()){
      (mlist++)->move = make_move(from,att.lsb<D>());
    }
  }
  //5-9
  const auto remain = get_not_prom_area(c) &(~(get_prom_area(c) | get_pre_prom_area(c)));
  tmp_piece = piece & remain;
  while(tmp_piece.is_not_zero()){
    const Square from = tmp_piece.lsb<D>();
    Bitboard att = pos.attack_from<c,SILVER>(from) & target;
    while(att.is_not_zero()){
      (mlist++)->move = make_move(from,att.lsb<D>());
    }
  }
  return mlist;
}
//knight
template <Color c, MoveType mt> inline MoveVal * gen_knight_move
(const Position & pos, MoveVal * mlist, const Bitboard & target){

  ASSERT(mlist != nullptr);
  ASSERT(pos.is_ok());
  Bitboard piece = pos.piece_bb(c,KNIGHT);
  while(piece.is_not_zero()){
    const Square from = piece.lsb<D>();
    Bitboard att = pos.attack_from<c,KNIGHT>(from) & target;
    while(att.is_not_zero()){
      const Square to = att.lsb<D>();
      const Rank rank = square_to_rank(to);
      if( mt != QUIET_MOVE && is_include_rank<c,RANK_2>(rank)){
        (mlist++)->move = make_prom_move(from,to);
      }else if(is_include_rank<c,RANK_3>(rank)){
        if(mt == CAPTURE_MOVE || mt == CAPTURE_MOVE2){
          (mlist++)->move = make_prom_move(from,to);
          if(pos.square(to)){
            (mlist++)->move = make_move(from,to);
          }
        }else if(mt == EVASION_MOVE){
          (mlist++)->move = make_prom_move(from,to);
          (mlist++)->move = make_move(from,to);
        }else{
          (mlist++)->move = make_move(from,to);
        }
      }else{
        (mlist++)->move = make_move(from,to);
      }
    }
  }
  return mlist;
}
//lance
template <Color c, MoveType mt> inline MoveVal * gen_lance_move
(const Position & pos, MoveVal * mlist, const Bitboard & target){

  ASSERT(mlist != nullptr);
  ASSERT(pos.is_ok());
  Bitboard piece = pos.piece_bb(c,LANCE);
  Bitboard att;
  //right
  while(piece.bb(0)){
    const Square from = piece.lsb_right<D>();
    att.set(get_lance_attack(c,from,0,pos.occ_bb()) & target.bb(0),0ull);
    while(att.bb(0)){
      const Square to = att.lsb_right<D>();
      const Rank rank = square_to_rank(to);
      if( mt != QUIET_MOVE && is_include_rank<c,RANK_2>(rank)){
        (mlist++)->move = make_prom_move(from,to);
      }else if(is_include_rank<c,RANK_3>(rank)){
        if(mt == CAPTURE_MOVE || mt == CAPTURE_MOVE2){
          (mlist++)->move = make_prom_move(from,to);
          if(pos.square(to)){
            (mlist++)->move = make_move(from,to);
          }
        }else if(mt == EVASION_MOVE){
          (mlist++)->move = make_prom_move(from,to);
          (mlist++)->move = make_move(from,to);
        }else{
          (mlist++)->move = make_move(from,to);
        }
      }else{
        (mlist++)->move = make_move(from,to);
      }
    }
  }
  //left
  while(piece.bb(1)){
    const Square from = piece.lsb_left<D>();
    att.set(0ull,get_lance_attack(c,from,1,pos.occ_bb())&target.bb(1));
    while(att.bb(1)){
      const Square to = att.lsb_left<D>();
      const Rank rank = square_to_rank(to);
      if( mt != QUIET_MOVE && is_include_rank<c,RANK_2>(rank)){
        (mlist++)->move = make_prom_move(from,to);
      }else if(is_include_rank<c,RANK_3>(rank)){
        if(mt == CAPTURE_MOVE || mt == CAPTURE_MOVE2){
          (mlist++)->move = make_prom_move(from,to);
          if(pos.square(to)){
            (mlist++)->move = make_move(from,to);
          }
        }else if(mt == EVASION_MOVE){
          (mlist++)->move = make_prom_move(from,to);
          (mlist++)->move = make_move(from,to);
        }else{
          (mlist++)->move = make_move(from,to);
        }
      }else{
        (mlist++)->move = make_move(from,to);
      }
    }
  }
  return mlist;
}
#define MAKE_DROP_MOVE_LET_RBGS_NUM do{\
  switch(rbgs_num){\
    case 0:\
            ASSERT(p1==HAND_NULL_H); \
            ASSERT(p2==HAND_NULL_H); \
            ASSERT(p3==HAND_NULL_H); \
    break;\
    case 1: \
            ASSERT(p1!=HAND_NULL_H); \
            ASSERT(p2==HAND_NULL_H); \
            ASSERT(p3==HAND_NULL_H); \
    (mlist++)->move = make_drop_move(to,p1); \
    break;\
    case 2: \
            ASSERT(p1!=HAND_NULL_H); \
            ASSERT(p2!=HAND_NULL_H); \
            ASSERT(p3==HAND_NULL_H); \
    (mlist++)->move = make_drop_move(to,p1); \
    (mlist++)->move = make_drop_move(to,p2); \
    break;\
    case 3:\
           ASSERT(p1!=HAND_NULL_H); \
           ASSERT(p2!=HAND_NULL_H); \
           ASSERT(p3!=HAND_NULL_H); \
           (mlist++)->move = make_drop_move(to,p1); \
           (mlist++)->move = make_drop_move(to,p2); \
           (mlist++)->move = make_drop_move(to,p3); \
    break;\
    case 4:\
           ASSERT(p1!=HAND_NULL_H); \
          ASSERT(p2!=HAND_NULL_H); \
          ASSERT(p3!=HAND_NULL_H); \
          (mlist++)->move = make_drop_move(to,ROOK_H); \
          (mlist++)->move = make_drop_move(to,BISHOP_H); \
          (mlist++)->move = make_drop_move(to,GOLD_H); \
          (mlist++)->move = make_drop_move(to,SILVER_H); \
    break;\
    default:\
            ASSERT(false);\
    break;\
  }\
}while(0)
template<Color c, int rbgs_num, bool have_pawn, bool have_lance, bool have_knight>
static inline MoveVal * add_rbgs_drop_move
(const Position & pos, MoveVal * mlist, const Bitboard & target, const HandPiece p1, const HandPiece p2, const HandPiece p3){

  ASSERT(mlist != nullptr);
  ASSERT(pos.is_ok());

  Bitboard pawn_bb = pos.piece_bb(c,PAWN);
  bool pawn_flag[FILE_SIZE];

  ASSERT(mask[ 0].bb(0));
  ASSERT(mask[ 9].bb(0));
  ASSERT(mask[18].bb(0));
  ASSERT(mask[27].bb(0));
  ASSERT(mask[36].bb(0));
  ASSERT(mask[45].bb(0));
  ASSERT(mask[54].bb(0));
  ASSERT(mask[63].bb(1));
  ASSERT(mask[72].bb(1));
  //generate can drop file flag
  pawn_flag[FILE_A] = (pawn_bb.bb(0) & file_mask[FILE_A].bb(0)) == 0;
  pawn_flag[FILE_B] = (pawn_bb.bb(0) & file_mask[FILE_B].bb(0)) == 0;
  pawn_flag[FILE_C] = (pawn_bb.bb(0) & file_mask[FILE_C].bb(0)) == 0;
  pawn_flag[FILE_D] = (pawn_bb.bb(0) & file_mask[FILE_D].bb(0)) == 0;
  pawn_flag[FILE_E] = (pawn_bb.bb(0) & file_mask[FILE_E].bb(0)) == 0;
  pawn_flag[FILE_F] = (pawn_bb.bb(0) & file_mask[FILE_F].bb(0)) == 0;
  pawn_flag[FILE_G] = (pawn_bb.bb(0) & file_mask[FILE_G].bb(0)) == 0;
  pawn_flag[FILE_H] = (pawn_bb.bb(1) & file_mask[FILE_H].bb(1)) == 0;
  pawn_flag[FILE_I] = (pawn_bb.bb(1) & file_mask[FILE_I].bb(1)) == 0;

  Bitboard to_rank1,to_rank2,to_remain;
  if(c == BLACK){
    to_rank1 = target & rank_mask[RANK_1];
    to_rank2 = target & rank_mask[RANK_2];
  }else{
    to_rank1 = target & rank_mask[RANK_9];
    to_rank2 = target & rank_mask[RANK_8];
  }
  to_remain = target & (~(to_rank1 | to_rank2));
  Square to;
  //rank1
  BB_FOR_EACH(to_rank1,to,
      MAKE_DROP_MOVE_LET_RBGS_NUM;
      );
  //rank2
  BB_FOR_EACH(to_rank2,to,
      MAKE_DROP_MOVE_LET_RBGS_NUM;
      if(have_lance)
        (mlist++)->move = make_drop_move(to,LANCE_H);
      if(have_pawn
        && pawn_flag[square_to_file(to)]
        && !pos.move_is_mate_with_pawn_drop(to))
        (mlist++)->move = make_drop_move(to,PAWN_H);
      );
  //remain
  BB_FOR_EACH(to_remain,to,
      MAKE_DROP_MOVE_LET_RBGS_NUM;
      if(have_knight)
        (mlist++)->move = make_drop_move(to,KNIGHT_H);
      if(have_lance)
        (mlist++)->move = make_drop_move(to,LANCE_H);
      if(have_pawn
      && pawn_flag[square_to_file(to)]
      && !pos.move_is_mate_with_pawn_drop(to))
        (mlist++)->move = make_drop_move(to,PAWN_H);
      );
  return mlist;
}
#undef MAKE_DROP_MOVE_LET_RBGS_NUM
template<Color c, bool have_pawn, bool have_lance, bool have_knight>
static inline MoveVal * add_rbgs_drop_move
(const Position & pos, MoveVal * mlist, const Bitboard & target, const Hand & h){

  ASSERT(mlist != nullptr);
  ASSERT(pos.is_ok());
  if(h.is_have(ROOK_H)){
    if(h.is_have(BISHOP_H)){
      if(h.is_have(GOLD_H)){
        if(h.is_have(SILVER_H)){
          //rbgs
          return add_rbgs_drop_move<c,4,have_pawn,have_lance,have_knight>
            (pos,mlist,target,ROOK_H,BISHOP_H,GOLD_H);
        }else{
          //rbg
          return add_rbgs_drop_move<c,3,have_pawn,have_lance,have_knight>
            (pos,mlist,target,ROOK_H,BISHOP_H,GOLD_H);
        }
      }else{
        if(h.is_have(SILVER_H)){
          //rbs
          return add_rbgs_drop_move<c,3,have_pawn,have_lance,have_knight>
            (pos,mlist,target,ROOK_H,BISHOP_H,SILVER_H);
        }else{
          //rb
          return add_rbgs_drop_move<c,2,have_pawn,have_lance,have_knight>
            (pos,mlist,target,ROOK_H,BISHOP_H,HAND_NULL_H);
        }
      }
    }else{
      if(h.is_have(GOLD_H)){
        if(h.is_have(SILVER_H)){
          //rgs
          return add_rbgs_drop_move<c,3,have_pawn,have_lance,have_knight>
            (pos,mlist,target,ROOK_H,GOLD_H,SILVER_H);
        }else{
          //rg
          return add_rbgs_drop_move<c,2,have_pawn,have_lance,have_knight>
            (pos,mlist,target,ROOK_H,GOLD_H,HAND_NULL_H);
        }
      }else{
        if(h.is_have(SILVER_H)){
          //rs
          return add_rbgs_drop_move<c,2,have_pawn,have_lance,have_knight>
            (pos,mlist,target,ROOK_H,SILVER_H,HAND_NULL_H);
        }else{
          //r
          return add_rbgs_drop_move<c,1,have_pawn,have_lance,have_knight>
            (pos,mlist,target,ROOK_H,HAND_NULL_H,HAND_NULL_H);
        }
      }
    }
  }else{
    if(h.is_have(BISHOP_H)){
      if(h.is_have(GOLD_H)){
        if(h.is_have(SILVER_H)){
          //bgs
          return add_rbgs_drop_move<c,3,have_pawn,have_lance,have_knight>
            (pos,mlist,target,BISHOP_H,GOLD_H,SILVER_H);
        }else{
          //bg
          return add_rbgs_drop_move<c,2,have_pawn,have_lance,have_knight>
            (pos,mlist,target,BISHOP_H,GOLD_H,HAND_NULL_H);
        }
      }else{
        if(h.is_have(SILVER_H)){
          //bs
          return add_rbgs_drop_move<c,2,have_pawn,have_lance,have_knight>
            (pos,mlist,target,BISHOP_H,SILVER_H,HAND_NULL_H);
        }else{
          //b
          return add_rbgs_drop_move<c,1,have_pawn,have_lance,have_knight>
            (pos,mlist,target,BISHOP_H,HAND_NULL_H,HAND_NULL_H);
        }
      }
    }else{
      if(h.is_have(GOLD_H)){
        if(h.is_have(SILVER_H)){
          //gs
          return add_rbgs_drop_move<c,2,have_pawn,have_lance,have_knight>
            (pos,mlist,target,GOLD_H,SILVER_H,HAND_NULL_H);
        }else{
          //g
          return add_rbgs_drop_move<c,1,have_pawn,have_lance,have_knight>
            (pos,mlist,target,GOLD_H,HAND_NULL_H,HAND_NULL_H);
        }
      }else{
        if(h.is_have(SILVER_H)){
          //s
          return add_rbgs_drop_move<c,1,have_pawn,have_lance,have_knight>
            (pos,mlist,target,SILVER_H,HAND_NULL_H,HAND_NULL_H);
        }else{
          //nothing
          return add_rbgs_drop_move<c,0,have_pawn,have_lance,have_knight>
            (pos,mlist,target,HAND_NULL_H,HAND_NULL_H,HAND_NULL_H);
        }
      }
    }
  }
}
template<Color c, bool have_pawn, bool have_lance>
static inline MoveVal * add_knight_drop_move
(const Position & pos, MoveVal * mlist, const Bitboard & target, const Hand & h){
  ASSERT(mlist != nullptr);
  ASSERT(pos.is_ok());
  if(h.is_have(KNIGHT_H)){
    return add_rbgs_drop_move<c,have_pawn,have_lance,true>(pos,mlist,target,h);
  }else{
    return add_rbgs_drop_move<c,have_pawn,have_lance,false>(pos,mlist,target,h);
  }
}
template<Color c,bool have_pawn>
static inline  MoveVal * add_lance_drop_move
(const Position & pos, MoveVal * mlist, const Bitboard & target, const Hand & h){
  ASSERT(mlist != nullptr);
  ASSERT(pos.is_ok());
  if(h.is_have(LANCE_H)){
    return add_knight_drop_move<c,have_pawn,true>(pos,mlist,target,h);
  }else{
    return add_knight_drop_move<c,have_pawn,false>(pos,mlist,target,h);
  }
}
template<Color c> MoveVal * gen_drop(const Position & pos, MoveVal * mlist, const Bitboard & target){
  ASSERT(mlist != nullptr);
  ASSERT(pos.is_ok());
  const Hand hand = pos.hand(c);
  if(hand.is_have(PAWN_H)){
    return add_lance_drop_move<c,true>(pos,mlist,target,hand);
  }else{
    return add_lance_drop_move<c,false>(pos,mlist,target,hand);
  }
}
template<Color c, MoveType mt>class GenMove : boost::noncopyable{
  public:
    inline MoveVal * operator ()(const Position & pos, MoveVal * mlist)const{
      if(false){
        //dummy
    	  return mlist;
      }else if(mt == CAPTURE_MOVE){
        ASSERT(!pos.in_checked());
        const Bitboard cap_bb = pos.color_bb(opp_color(c));
        const Bitboard cap_prom_bb =  cap_bb | (~pos.color_bb(c) & get_prom_area(c));
        mlist = gen_pawn_move<c,mt>(pos,mlist,cap_prom_bb);
        mlist = gen_lance_move<c,mt>(pos,mlist,cap_prom_bb);
        mlist = gen_knight_move<c,mt>(pos,mlist,cap_prom_bb);
        mlist = gen_silver_move<c,mt>(pos,mlist,cap_bb);
        mlist = gen_no_prom_move<c,GOLDS,mt>(pos,mlist,cap_bb);
        mlist = gen_king_move<c,mt>(pos,mlist,cap_bb);
        mlist = gen_bishop_rook_move<c,BISHOP,mt>(pos,mlist,cap_prom_bb);
        mlist = gen_bishop_rook_move<c,ROOK,mt>(pos,mlist,cap_prom_bb);
        mlist = gen_no_prom_move<c,PBISHOP,mt>(pos,mlist,cap_bb);
        mlist = gen_no_prom_move<c,PROOK,mt>(pos,mlist,cap_bb);
        return mlist;
      }else if(mt == QUIET_MOVE){
        ASSERT(!pos.in_checked());
        const Bitboard no_prom_1to9_bb = ~pos.occ_bb();
        const Bitboard no_prom_3to9_bb = no_prom_1to9_bb & (get_not_prom_area(c) | get_rank3_area(c));
        const Bitboard no_prom_4to9_bb = no_prom_1to9_bb & get_not_prom_area(c);
        mlist = gen_pawn_move<c,mt>(pos,mlist,no_prom_4to9_bb);
        mlist = gen_lance_move<c,mt>(pos,mlist,no_prom_3to9_bb);
        mlist = gen_knight_move<c,mt>(pos,mlist,no_prom_3to9_bb);
        mlist = gen_silver_move<c,mt>(pos,mlist,no_prom_1to9_bb);
        mlist = gen_no_prom_move<c,GOLDS,mt>(pos,mlist,no_prom_1to9_bb);
        mlist = gen_king_move<c,mt>(pos,mlist,no_prom_1to9_bb);
        mlist = gen_bishop_rook_move<c,BISHOP,mt>(pos,mlist,no_prom_4to9_bb);
        mlist = gen_bishop_rook_move<c,ROOK,mt>(pos,mlist,no_prom_4to9_bb);
        mlist = gen_no_prom_move<c,PBISHOP,mt>(pos,mlist,no_prom_1to9_bb);
        mlist = gen_no_prom_move<c,PROOK,mt>(pos,mlist,no_prom_1to9_bb);
        return mlist;
      }else if(mt == DROP_MOVE){
        ASSERT(!pos.in_checked());
        const Bitboard target = (~pos.occ_bb()) & ALL_ONE_BB;
        return gen_drop<c>(pos,mlist,target);
      }else if(mt == EVASION_MOVE){
        ASSERT(pos.in_checked());
        const auto checker = pos.checker_bb();
        ASSERT(checker.pop_cnt() != 0);
        ASSERT(checker.pop_cnt() < 3);
        Bitboard checker_attack(0,0);
        int cnum = 0;
        const auto opp = opp_color(c);
        Square csq;
        auto tmp_checker = checker;
        do{
          ASSERT(cnum >= 0);
          ASSERT(cnum <= 3);
          ++cnum;
          ASSERT(tmp_checker.is_not_zero());
          csq = tmp_checker.lsb<D>();
          const auto checker_pt = piece_to_piece_type(pos.square(csq));
          switch(checker_pt){//TODO
            case PAWN: /*checker_attack |= get_pseudo_attack<PAWN>(opp,csq);*/ break;
            case LANCE: checker_attack |= get_pseudo_attack<LANCE>(opp,csq); break;
            case KNIGHT: /*checker_attack |= get_pseudo_attack<KNIGHT>(opp,csq);*/ break;
            case SILVER: checker_attack |= get_pseudo_attack<SILVER>(opp,csq); break;
            case GOLD: case PPAWN: case PLANCE:
            case PKNIGHT: case PSILVER:
              checker_attack |= get_pseudo_attack<GOLD>(opp,csq); break;
            case KING: checker_attack |= get_pseudo_attack<KING>(opp,csq); break;
            case BISHOP: checker_attack |= get_pseudo_attack<BISHOP>(opp,csq); break;
            case ROOK: checker_attack |= get_pseudo_attack<ROOK>(opp,csq); break;
            case PBISHOP: checker_attack |= get_pseudo_attack<PBISHOP>(opp,csq); break;
            case PROOK:
              if(!get_between(pos.king_sq(c),csq).is_not_zero()
              && get_pseudo_attack<BISHOP>(opp,csq).is_seted(pos.king_sq(c))){
                checker_attack |= pos.attack_from<PROOK>(opp,csq);
              }else{
                checker_attack |= get_pseudo_attack<PROOK>(opp,csq);
              }
              break;
            default:
#ifndef NDEBUG
              printf("checker_pt %d csq %d\n",checker_pt,csq);
#endif
            ASSERT(false);
            break;
          }
        }while(tmp_checker.is_not_zero());
        ASSERT(cnum >= 1);
        ASSERT(cnum <= 2);
        ASSERT(square_is_ok(csq));
        Bitboard target = ((~pos.color_bb(c)) & (~checker_attack)) | checker;
        mlist = gen_king_move<c,mt>(pos,mlist,target);
        if(cnum == 2){
          return mlist;
        }
        const Bitboard chuai = get_between(pos.king_sq(c),csq);
        const Bitboard cap_and_chuai = checker | chuai;
        mlist = gen_pawn_move<c,mt>(pos,mlist,cap_and_chuai);
        mlist = gen_lance_move<c,mt>(pos,mlist,cap_and_chuai);
        mlist = gen_knight_move<c,mt>(pos,mlist,cap_and_chuai);
        mlist = gen_silver_move<c,mt>(pos,mlist,cap_and_chuai);
        mlist = gen_no_prom_move<c,GOLDS,mt>(pos,mlist,cap_and_chuai);
        mlist = gen_bishop_rook_move<c,BISHOP,mt>(pos,mlist,cap_and_chuai);
        mlist = gen_bishop_rook_move<c,ROOK,mt>(pos,mlist,cap_and_chuai);
        mlist = gen_no_prom_move<c,PBISHOP,mt>(pos,mlist,cap_and_chuai);
        mlist = gen_no_prom_move<c,PROOK,mt>(pos,mlist,cap_and_chuai);
        mlist = gen_drop<c>(pos,mlist,chuai);
        return mlist;
      }else if(mt == CAPTURE_MOVE2){
        ASSERT(!pos.in_checked());
        const Bitboard cap_bb = pos.color_bb(opp_color(c));
        const Bitboard cap_prom_bb =  cap_bb | (~pos.color_bb(c) & get_prom_area(c));
        mlist = gen_pawn_move<c,mt>(pos,mlist,cap_prom_bb);
        mlist = gen_lance_move<c,mt>(pos,mlist,cap_bb);
        mlist = gen_knight_move<c,mt>(pos,mlist,cap_bb);
        mlist = gen_silver_move<c,mt>(pos,mlist,cap_bb);
        mlist = gen_no_prom_move<c,GOLDS,mt>(pos,mlist,cap_bb);
        mlist = gen_king_move<c,mt>(pos,mlist,cap_bb);
        mlist = gen_bishop_rook_move<c,BISHOP,mt>(pos,mlist,cap_bb);
        mlist = gen_bishop_rook_move<c,ROOK,mt>(pos,mlist,cap_bb);
        mlist = gen_no_prom_move<c,PBISHOP,mt>(pos,mlist,cap_bb);
        mlist = gen_no_prom_move<c,PROOK,mt>(pos,mlist,cap_bb);
        return mlist;
      }else{
        std::cout<<"error " <<mt<<std::endl;
        ASSERT(false);
        return mlist;
      }
    }
};
template<Color c, MoveType mt>MoveVal * gen_move(const Position & pos, MoveVal * mlist){

  if(c == BLACK){
    if(mt == CAPTURE_MOVE){
      return GenMove<BLACK, CAPTURE_MOVE>()(pos, mlist);
    }else if(mt == QUIET_MOVE){
      return GenMove<BLACK, QUIET_MOVE>()(pos, mlist);
    }else if(mt == DROP_MOVE){
      return GenMove<BLACK, DROP_MOVE>()(pos, mlist);
    }else if(mt == EVASION_MOVE){
      return GenMove<BLACK, EVASION_MOVE>()(pos, mlist);
    }
  }else{
    if(mt == CAPTURE_MOVE){
      return GenMove<WHITE, CAPTURE_MOVE>()(pos, mlist);
    }else if(mt == QUIET_MOVE){
      return GenMove<WHITE, QUIET_MOVE>()(pos, mlist);
    }else if(mt == DROP_MOVE){
      return GenMove<WHITE, DROP_MOVE>()(pos, mlist);
    }else if(mt == EVASION_MOVE){
      return GenMove<WHITE, EVASION_MOVE>()(pos, mlist);
    }
  }
  ASSERT(false);
  return mlist;
}
template<MoveType mt> MoveVal * gen_move(const Position & pos, MoveVal * mlist){
  return (pos.turn()) ? GenMove<WHITE, mt>()(pos, mlist) : GenMove<BLACK, mt>()(pos, mlist);
}
template<> MoveVal * gen_move<LEGAL_MOVE>(const Position & pos, MoveVal * mlist){
  MoveVal moves[MAX_MOVE_NUM];
  MoveVal *pm = moves;
  if(pos.in_checked()){
    pm = gen_move<EVASION_MOVE>(pos,pm);
  }else{
    pm = gen_move<CAPTURE_MOVE>(pos,pm);
    pm = gen_move<QUIET_MOVE>(pos,pm);
    pm = gen_move<DROP_MOVE>(pos,pm);
  }
  CheckInfo ci(pos);
  for(MoveVal * p = moves; p != pm; ++p){
    if(pos.pseudo_is_legal(p->move,ci.pinned())){
      (*mlist++) = *p;
    }
  }
  return mlist;
}

//generate check
// noprom move ppawn,plance,pknight,psilver,pbishop,prook,gold
template <Color c, PieceType pt> inline MoveVal * gen_direct_check_no_prom_move
(const Position & pos, MoveVal * mlist, const CheckInfo * ci){

  ASSERT(!can_prom_piece_type(pt));
  ASSERT(pt != KING);
  ASSERT(mlist != nullptr);
  ASSERT(pos.is_ok());
  Bitboard piece = (pt == GOLDS) ? pos.golds_bb(c)
                                 : pos.piece_bb(c,pt);
  constexpr PieceType pt2 = (pt == GOLDS) ? GOLD : pt;
  Bitboard target = ci->check_sq(pt2) & (~pos.color_bb(c));

  while(piece.is_not_zero()){
    const Square from = piece.lsb<D>();
    Bitboard att = pos.attack_from<c,pt2>(from) & target;
    while(att.is_not_zero()){
      (mlist++)->move = make_move(from,att.lsb<D>());
    }
  }
  return mlist;
}
//king
//king's direct check is nothing!
//pawn
template<Color c> inline MoveVal * gen_direct_check_pawn_move
(const Position & pos, MoveVal * mlist, const CheckInfo * ci){

  ASSERT(mlist != nullptr);
  ASSERT(pos.is_ok());
  const Bitboard piece = pos.piece_bb(c,PAWN);
  const Bitboard target = ((get_prom_area(c)     & ci->check_sq(GOLD))
                          |(get_not_prom_area(c) & ci->check_sq(PAWN)))
                          & (~pos.color_bb(c));
  Bitboard att = get_pawn_attack<c>(piece) & target;
  Bitboard tmp_att = att & get_prom_area(c);
  Square to;
  //prom
  BB_FOR_EACH(tmp_att,to,
      const Square from = (c == BLACK) ? (to + DOWN) : (to + UP);
      (mlist++)->move = make_prom_move(from,to);
      );
  //no prom
  tmp_att = att & get_not_prom_area(c);
  BB_FOR_EACH(tmp_att,to,
      const Square from = (c == BLACK) ? (to + DOWN) : (to + UP);
      (mlist++)->move = make_move(from,to);
      );
  return mlist;
}
// bishop,rook
template <Color c, PieceType pt> inline MoveVal * gen_direct_check_bishop_rook_move
(const Position & pos, MoveVal * mlist, const CheckInfo * ci){

  ASSERT(mlist != nullptr);
  ASSERT(pos.is_ok());
  Bitboard piece = pos.piece_bb(c,pt);
  Bitboard tmp_piece = piece & get_prom_area(c);
  piece &= get_not_prom_area(c);
  const Bitboard target_1to3 = ci->check_sq(prom_piece_type(pt)) & (~pos.color_bb(c));
  const Bitboard target_4to9 = ((ci->check_sq(prom_piece_type(pt)) & get_prom_area(c))
                               |(ci->check_sq(pt) & get_not_prom_area(c)))
                                & (~pos.color_bb(c));
  //1-3
  while(tmp_piece.is_not_zero()){
    const Square from = tmp_piece.lsb<D>();
    Bitboard att = pos.attack_from<c,pt>(from) & target_1to3;
    while(att.is_not_zero()){
      (mlist++)->move = make_prom_move(from,att.lsb<D>());
    }
  }
  //4-9
  while(piece.is_not_zero()){
    const Square from = piece.lsb<D>();
    Bitboard att = pos.attack_from<c,pt>(from) & target_4to9;
    Bitboard tmp_att = att & get_prom_area(c);
    att &= get_not_prom_area(c);
    //prom
    while(tmp_att.is_not_zero()){
      (mlist++)->move = make_prom_move(from,tmp_att.lsb<D>());
    }
    //no prom
    while(att.is_not_zero()){
      (mlist++)->move = make_move(from,att.lsb<D>());
    }
  }
  return mlist;
}
//silver
template <Color c> inline MoveVal * gen_direct_check_silver_move
(const Position & pos, MoveVal * mlist, const CheckInfo * ci){

  ASSERT(mlist != nullptr);
  ASSERT(pos.is_ok());
  //noprom
  auto piece = pos.piece_bb(c,SILVER);
  auto target = ci->check_sq(SILVER) & (~pos.color_bb(c));
  while(piece.is_not_zero()){
    const auto from = piece.lsb<D>();
    Bitboard att = pos.attack_from<c,SILVER>(from) & target;
    while(att.is_not_zero()){
      (mlist++)->move = make_move(from,att.lsb<D>());
    }
  }
  //prom
  piece = pos.piece_bb(c,SILVER) & (get_prom_area(c)
                                  | get_pre_prom_area(c));
  target = ci->check_sq(PSILVER)
        & (~pos.color_bb(c))
        & (get_prom_area(c) | get_pre_prom_area(c));
  while(piece.is_not_zero()){
    const auto from = piece.lsb<D>();
    Bitboard att = pos.attack_from<c,SILVER>(from) & target;
    while(att.is_not_zero()){
      (mlist++)->move = make_prom_move(from,att.lsb<D>());
    }
  }
  return mlist;
}
//knight
template <Color c> inline MoveVal * gen_direct_check_knight_move
(const Position & pos, MoveVal * mlist, const CheckInfo * ci){

  ASSERT(mlist != nullptr);
  ASSERT(pos.is_ok());
  //noprom
  auto piece = pos.piece_bb(c,KNIGHT);
  auto target = ci->check_sq(KNIGHT) & (~pos.color_bb(c));
  while(piece.is_not_zero()){
    const auto from = piece.lsb<D>();
    Bitboard att = pos.attack_from<c,KNIGHT>(from) & target;
    while(att.is_not_zero()){
      (mlist++)->move = make_move(from,att.lsb<D>());
    }
  }
  //prom
  piece = pos.piece_bb(c,KNIGHT) & (get_prom_area(c)
                                  | get_pre_prom_area(c)
                                  | get_middle_area());
  target = ci->check_sq(PKNIGHT)
        & (~pos.color_bb(c))
        & get_prom_area(c);
  while(piece.is_not_zero()){
    const auto from = piece.lsb<D>();
    Bitboard att = pos.attack_from<c,KNIGHT>(from) & target;
    while(att.is_not_zero()){
      (mlist++)->move = make_prom_move(from,att.lsb<D>());
    }
  }
  return mlist;
}
//lance
template <Color c> inline MoveVal * gen_direct_check_lance_move
(const Position & pos, MoveVal * mlist, const CheckInfo * ci){

  ASSERT(mlist != nullptr);
  ASSERT(pos.is_ok());
  auto piece = pos.piece_bb(c,LANCE);
  //noprom
  Bitboard checker = piece & get_pseudo_attack<LANCE>(opp_color(c),ci->king_sq());
  while(checker.is_not_zero()){
    const auto from = checker.lsb<D>();
    Bitboard line_bb = get_between(ci->king_sq(),from) & pos.occ_bb();
    if(line_bb.pop_cnt() == 1){
      const auto to = line_bb.lsb<N>();
      const auto rank = square_to_rank(to);
      if(!is_include_rank<c,RANK_2>(rank) && piece_color(pos.square(to)) != c){
        (mlist++)->move = make_move(from,to);
      }
    }
  }
  //prom
  piece = pos.piece_bb(c,LANCE);
  auto target = ci->check_sq(PLANCE)
                & (~pos.color_bb(c))
                & get_prom_area(c);
  while(piece.is_not_zero()){
    const auto from = piece.lsb<D>();
    Bitboard att = pos.attack_from<c,LANCE>(from) & target;
    while(att.is_not_zero()){
      (mlist++)->move = make_prom_move(from,att.lsb<D>());
    }
  }
  return mlist;
}
template<Color c, bool mate_three_flag> MoveVal * gen_check_drop(const Position & pos, MoveVal * mlist, const CheckInfo * ci){

  //mate_tree_
  const Hand h = pos.hand(c);

  if(h.is_have(PAWN_H)){
    auto target = ci->check_sq(PAWN) & (~pos.occ_bb());
    auto file = square_to_file(ci->king_sq());
    auto pawn_bb = pos.piece_bb(c,PAWN);
    bool double_pawn = (pawn_bb & file_mask[file]).is_not_zero();
    if(target.is_not_zero() && !double_pawn){
      const auto to = target.lsb<N>();
      if(!pos.move_is_mate_with_pawn_drop(to)){
        (mlist++)->move = make_drop_move(to,PAWN_H);
      }
    }
  }
  if(h.is_have(LANCE_H)){
    if(mate_three_flag){
      constexpr auto end_rank = c ? RANK_1 : RANK_9;
      constexpr auto inc_sq   = c ?      UP :      DOWN;
      constexpr auto inc_rank = c ? RANK_UP : RANK_DOWN;
      auto rank = square_to_rank(ci->king_sq());
      auto to   = ci->king_sq() + inc_sq;
      for( int dist = 0;
          rank != end_rank && !pos.square(to) && dist < 3;
          to += inc_sq, rank += inc_rank, ++dist){
        (mlist++)->move = make_drop_move(to,LANCE_H);
      }
    }else{
      auto target = ci->check_sq(LANCE) & (~pos.occ_bb());
      while(target.is_not_zero()){
        (mlist++)->move = make_drop_move(target.lsb<D>(),LANCE_H);
      }
    }
  }
  if(h.is_have(KNIGHT_H)){
    auto target = ci->check_sq(KNIGHT) & (~pos.occ_bb());
    while(target.is_not_zero()){
      (mlist++)->move = make_drop_move(target.lsb<D>(),KNIGHT_H);
    }
  }
  if(h.is_have(SILVER_H)){
    auto target = ci->check_sq(SILVER) & (~pos.occ_bb());
    while(target.is_not_zero()){
      (mlist++)->move = make_drop_move(target.lsb<D>(),SILVER_H);
    }
  }
  if(h.is_have(GOLD_H)){
    auto target = ci->check_sq(GOLD) & (~pos.occ_bb());
    while(target.is_not_zero()){
      (mlist++)->move = make_drop_move(target.lsb<D>(),GOLD_H);
    }
  }
  if(h.is_have(BISHOP_H)){
    if(mate_three_flag){
      {//右上
        constexpr auto end_rank = c ?    RANK_9  :     RANK_1;
        constexpr auto end_file = c ?    FILE_I  :     FILE_A;
        constexpr auto inc_rank = c ? RANK_DOWN  :    RANK_UP;
        constexpr auto inc_file = c ? FILE_LEFT  : FILE_RIGHT;
        constexpr auto inc_sq   = c ?  LEFT_DOWN :   RIGHT_UP;
        auto rank = square_to_rank(ci->king_sq());
        auto file = square_to_file(ci->king_sq());
        auto to   = ci->king_sq() + inc_sq;
        for( int dist = 0;
            rank != end_rank && file != end_file && !pos.square(to) && dist < 3;
            to += inc_sq, rank += inc_rank, file += inc_file, ++dist){
          (mlist++)->move = make_drop_move(to,BISHOP_H);
        }
      }
      {//左上
        constexpr auto end_rank = c ?     RANK_9  :    RANK_1;
        constexpr auto end_file = c ?     FILE_A  :    FILE_I;
        constexpr auto inc_rank = c ?  RANK_DOWN  :   RANK_UP;
        constexpr auto inc_file = c ? FILE_RIGHT  : FILE_LEFT;
        constexpr auto inc_sq   = c ?  RIGHT_DOWN :   LEFT_UP;
        auto rank = square_to_rank(ci->king_sq());
        auto file = square_to_file(ci->king_sq());
        auto to   = ci->king_sq() + inc_sq;
        for( int dist = 0;
            rank != end_rank && file != end_file && !pos.square(to) && dist < 3;
            to += inc_sq, rank += inc_rank, file += inc_file, ++dist){
          (mlist++)->move = make_drop_move(to,BISHOP_H);
        }
      }
      {//右下
        constexpr auto end_rank = c ?    RANK_1 :     RANK_9;
        constexpr auto end_file = c ?    FILE_I :     FILE_A;
        constexpr auto inc_rank = c ?   RANK_UP :  RANK_DOWN;
        constexpr auto inc_file = c ? FILE_LEFT : FILE_RIGHT;
        constexpr auto inc_sq   = c ?  LEFT_UP  : RIGHT_DOWN;
        auto rank = square_to_rank(ci->king_sq());
        auto file = square_to_file(ci->king_sq());
        auto to   = ci->king_sq() + inc_sq;
        for( int dist = 0;
            rank != end_rank && file != end_file && !pos.square(to) && dist < 3;
            to += inc_sq, rank += inc_rank, file += inc_file, ++dist){
          (mlist++)->move = make_drop_move(to,BISHOP_H);
        }
      }
      {//左下
        constexpr auto end_rank = c ?     RANK_1 :    RANK_9;
        constexpr auto end_file = c ?     FILE_A :    FILE_I;
        constexpr auto inc_rank = c ?    RANK_UP : RANK_DOWN;
        constexpr auto inc_file = c ? FILE_RIGHT : FILE_LEFT;
        constexpr auto inc_sq   = c ?  RIGHT_UP  : LEFT_DOWN;
        auto rank = square_to_rank(ci->king_sq());
        auto file = square_to_file(ci->king_sq());
        auto to   = ci->king_sq() + inc_sq;
        for( int dist = 0;
            rank != end_rank && file != end_file && !pos.square(to) && dist < 3;
            to += inc_sq, rank += inc_rank, file += inc_file, ++dist){
          (mlist++)->move = make_drop_move(to,BISHOP_H);
        }
      }
    }else{
      auto target = ci->check_sq(BISHOP) & (~pos.occ_bb());
      while(target.is_not_zero()){
        (mlist++)->move = make_drop_move(target.lsb<D>(),BISHOP_H);
      }
    }
  }
  if(h.is_have(ROOK_H)){
    if(mate_three_flag){
      {//上
        constexpr auto end_rank = c ?    RANK_9 :  RANK_1;
        constexpr auto inc_sq   = c ?       DOWN:      UP;
        constexpr auto inc_rank = c ? RANK_DOWN : RANK_UP;
        auto rank = square_to_rank(ci->king_sq());
        auto to   = ci->king_sq() + inc_sq;
        for( int dist = 0;
            rank != end_rank && !pos.square(to) && dist < 3;
            to += inc_sq, rank += inc_rank, ++dist){
          (mlist++)->move = make_drop_move(to,ROOK_H);
        }
      }
      {//下
        constexpr auto end_rank = c ?  RANK_1 :   RANK_9;
        constexpr auto inc_sq   = c ?      UP :      DOWN;
        constexpr auto inc_rank = c ? RANK_UP : RANK_DOWN;
        auto rank = square_to_rank(ci->king_sq());
        auto to   = ci->king_sq() + inc_sq;
        for( int dist = 0;
            rank != end_rank && !pos.square(to) && dist < 3;
            to += inc_sq, rank += inc_rank, ++dist){
          (mlist++)->move = make_drop_move(to,ROOK_H);
        }
      }
      {//左
        constexpr auto end_file = c ?     FILE_A :    FILE_I;
        constexpr auto inc_sq   = c ?      RIGHT :      LEFT;
        constexpr auto inc_file = c ? FILE_RIGHT : FILE_LEFT;
        auto file = square_to_file(ci->king_sq());
        auto to   = ci->king_sq() + inc_sq;
        for( int dist = 0;
            file != end_file && !pos.square(to) && dist < 3;
            to += inc_sq, file += inc_file, ++dist){
          (mlist++)->move = make_drop_move(to,ROOK_H);
        }
      }
      {//右
        constexpr auto end_file = c ?     FILE_I :    FILE_A;
        constexpr auto inc_sq   = c ?      LEFT  :     RIGHT;
        constexpr auto inc_file = c ? FILE_LEFT  : FILE_RIGHT;
        auto file = square_to_file(ci->king_sq());
        auto to   = ci->king_sq() + inc_sq;
        for( int dist = 0;
            file != end_file && !pos.square(to) && dist < 3;
            to += inc_sq, file += inc_file, ++dist){
          (mlist++)->move = make_drop_move(to,ROOK_H);
        }
      }
    }
    else{
      auto target = ci->check_sq(ROOK) & (~pos.occ_bb());
      while(target.is_not_zero()){
        (mlist++)->move = make_drop_move(target.lsb<D>(),ROOK_H);
      }
    }
  }
  return mlist;
}
template<Color c> MoveVal * gen_discover
  (const Position & pos, MoveVal * mlist, const CheckInfo * ci){

  Bitboard bb = ci->discover_checker();
  const auto ksq = ci->king_sq();
  while(bb.is_not_zero()){
    const auto from = bb.lsb<D>();
    const auto p = pos.square(from);
    const auto pc = piece_to_piece_type(p);
    const auto dir = get_square_relation(from,ksq);
    //TODO
    //　/の利きとか-の利きだけ残すためにアンドをとってる
    Bitboard line;
    switch(dir){
      case LEFT_UP_RELATION: case RIGHT_UP_RELATION:
      line = get_pseudo_attack<BISHOP>(opp_color(c),ksq)
                        & get_pseudo_attack<BISHOP>(opp_color(c),from);
      break;
      case FILE_RELATION: case RANK_RELATION:
      line = get_pseudo_attack<ROOK>(opp_color(c),ksq)
                        & get_pseudo_attack<ROOK>(opp_color(c),from);
      break;
      default :
      std::cout<<dir<<std::endl;
      std::cout<<from<<" "<<ksq<<std::endl;
      ASSERT(false);
      break;
    }
    const Bitboard pseudo = get_pseudo_attack(pc,opp_color(c),ksq);
    Bitboard att = pos.attack_from(p,from);
    Bitboard target = line | pseudo;
    target = (~pos.color_bb(c)) & (~target);
    att = att & target;
    while(att.is_not_zero()){
      Square to = att.lsb<D>();
      const bool prom = is_prom_square(from,c) | is_prom_square(to,c);
      if(pc == KNIGHT || pc == LANCE){
        const auto rank = square_to_rank(to);
        if(is_include_rank<c,RANK_2>(rank)){
          (mlist++)->move = make_prom_move(from,to);
        }else{
          if(is_include_rank<c,RANK_3>(rank)){
            (mlist++)->move = make_prom_move(from,to);
          }
          (mlist++)->move = make_move(from,to);
        }
      }else if(pc == ROOK || pc == BISHOP || pc == PAWN){
        if(prom){
          (mlist++)->move = make_prom_move(from,to);
        }else{
          (mlist++)->move = make_move(from,to);
        }
      }else if(pc == SILVER){
        if(prom){
          (mlist++)->move = make_prom_move(from,to);
        }
        (mlist++)->move = make_move(from,to);
      }else{
        (mlist++)->move = make_move(from,to);
      }
    }
  }
  return mlist;
}
template<Color c,bool mate_three_flag>MoveVal * gen_check(const Position & pos, MoveVal * mlist, const CheckInfo * ci){

  ASSERT(pos.is_ok());
  ASSERT(mlist != nullptr);
  ASSERT(ci->is_ok());

  mlist = gen_check_drop<c,mate_three_flag>(pos,mlist,ci);
  mlist = gen_direct_check_no_prom_move<c,PROOK>(pos,mlist,ci);
  mlist = gen_direct_check_no_prom_move<c,PBISHOP>(pos,mlist,ci);
  mlist = gen_direct_check_no_prom_move<c,GOLDS>(pos,mlist,ci);
  mlist = gen_direct_check_bishop_rook_move<c,ROOK>(pos,mlist,ci);
  mlist = gen_direct_check_bishop_rook_move<c,BISHOP>(pos,mlist,ci);
  mlist = gen_direct_check_silver_move<c>(pos,mlist,ci);
  mlist = gen_direct_check_knight_move<c>(pos,mlist,ci);
  mlist = gen_direct_check_lance_move<c>(pos,mlist,ci);
  mlist = gen_direct_check_pawn_move<c>(pos,mlist,ci);
  mlist = gen_discover<c>(pos,mlist,ci);

  return mlist;

}
template MoveVal * gen_move<CAPTURE_MOVE>(const Position & pos, MoveVal * mlist);
template MoveVal * gen_move<QUIET_MOVE>(const Position & pos, MoveVal * mlist);
template MoveVal * gen_move<DROP_MOVE>(const Position & pos, MoveVal * mlist);
template MoveVal * gen_move<EVASION_MOVE>(const Position & pos, MoveVal * mlist);
template MoveVal * gen_move<CAPTURE_MOVE2>(const Position & pos, MoveVal * mlist);

template MoveVal * gen_move<BLACK,CAPTURE_MOVE>(const Position & pos, MoveVal * mlist);
template MoveVal * gen_move<BLACK,CAPTURE_MOVE2>(const Position & pos, MoveVal * mlist);
template MoveVal * gen_move<BLACK,QUIET_MOVE>(const Position & pos, MoveVal * mlist);
template MoveVal * gen_move<BLACK,DROP_MOVE>(const Position & pos, MoveVal * mlist);
template MoveVal * gen_move<BLACK,EVASION_MOVE>(const Position & pos, MoveVal * mlist);

template MoveVal * gen_move<WHITE,CAPTURE_MOVE>(const Position & pos, MoveVal * mlist);
template MoveVal * gen_move<WHITE,CAPTURE_MOVE2>(const Position & pos, MoveVal * mlist);
template MoveVal * gen_move<WHITE,QUIET_MOVE>(const Position & pos, MoveVal * mlist);
template MoveVal * gen_move<WHITE,DROP_MOVE>(const Position & pos, MoveVal * mlist);
template MoveVal * gen_move<WHITE,EVASION_MOVE>(const Position & pos, MoveVal * mlist);

template MoveVal * gen_check<BLACK,true>(const Position & pos, MoveVal * mlist, const CheckInfo * ci);
template MoveVal * gen_check<WHITE,true>(const Position & pos, MoveVal * mlist, const CheckInfo * ci);
template MoveVal * gen_check<BLACK,false>(const Position & pos, MoveVal * mlist, const CheckInfo * ci);
template MoveVal * gen_check<WHITE,false>(const Position & pos, MoveVal * mlist, const CheckInfo * ci);
