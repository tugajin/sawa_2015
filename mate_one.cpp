#include "position.h"
#include "move.h"

template<PieceType pt> Bitboard get_target(const Color c, const Square ksq);

template<Color c>Move Position::mate_one(CheckInfo & ci){

  const auto hand = hand_[c];
  Move ret;
  if(hand.is_have(ROOK_H)){
    if(hand.is_have(BISHOP_H)){
      if(hand.is_have(GOLD_H)){
        ret = mate_one<c,true,true,true>(ci);
      }else{
        ret = mate_one<c,true,true,false>(ci);
      }
    }else{
      if(hand.is_have(GOLD_H)){
        ret = mate_one<c,true,false,true>(ci);
      }else{
        ret = mate_one<c,true,false,false>(ci);
      }
    }
  }else{
    if(hand.is_have(BISHOP_H)){
      if(hand.is_have(GOLD_H)){
        ret = mate_one<c,false,true,true>(ci);
      }else{
        ret = mate_one<c,false,true,false>(ci);
      }
    }else{
      if(hand.is_have(GOLD_H)){
        ret = mate_one<c,false,false,true>(ci);
      }else{
        ret = mate_one<c,false,false,false>(ci);
      }
    }
  }
  ASSERT(!ret.value() || mate_one_is_ok(ret));
  return ret;
}
template<Color c, bool have_rook, bool have_bishop, bool have_gold>Move Position::mate_one(const CheckInfo & ci){

  ASSERT(is_ok());
  ASSERT(ci.is_ok());
  ASSERT(c == turn());
  constexpr auto me = c;
  constexpr auto opp = opp_color(me);
  const auto hand = hand_[me];
  const auto opp_king_sq = ci.king_sq();
  const auto drop_target = (~occ_bb()) & ALL_ONE_BB;
  //drop
  Bitboard target;
  if(have_rook){
    target = get_target<ROOK>(me,opp_king_sq) & drop_target;
    while(target.is_not_zero()){
      const auto to = target.lsb<D>();
      if(!attack_to(me,to).is_not_zero()){
        continue;
      }
      operate_bb(me,to,ROOK);
      if(can_escape(opp,opp_king_sq,to,false)){
        operate_bb(me,to,ROOK);
        continue;
      }
      operate_bb(me,to,ROOK);
      return make_drop_move(to,ROOK_H);
    }
  }
  ASSERT(is_ok());
  if(!have_rook && hand.is_have(LANCE_H)){//９段目にいることは滅多にないので省略
    target = get_target<LANCE>(me,opp_king_sq) & drop_target;
    while(target.is_not_zero()){
      const auto to = target.lsb<D>();
      if(!attack_to(me,to).is_not_zero()){
        continue;
      }
      operate_bb(me,to,LANCE);
      if(can_escape(opp,opp_king_sq,to,false)){
        operate_bb(me,to,LANCE);
        continue;
      }
      operate_bb(me,to,LANCE);
      return make_drop_move(to,LANCE_H);
    }
  }
  ASSERT(is_ok());
  if(have_bishop){
    target = get_target<BISHOP>(me,opp_king_sq) & drop_target;
    while(target.is_not_zero()){
      const auto to = target.lsb<D>();
      if(!attack_to(me,to).is_not_zero()){
        continue;
      }
      operate_bb(me,to,BISHOP);
      if(can_escape(opp,opp_king_sq,to,false)){
        operate_bb(me,to,BISHOP);
        continue;
      }
      operate_bb(me,to,BISHOP);
      return make_drop_move(to,BISHOP_H);
    }
  }
  ASSERT(is_ok());
  if(have_gold){
    target = get_target<GOLD>(me,opp_king_sq) & drop_target;
    //尻金を消す
    if(have_rook){
      target &= (~get_pawn_attack(c,opp_king_sq));
    }
    while(target.is_not_zero()){
      const auto to = target.lsb<D>();
      if(!attack_to(me,to).is_not_zero()){
        continue;
      }
      operate_bb(me,to,GOLD);
      if(can_escape(opp,opp_king_sq,to,false)){
        operate_bb(me,to,GOLD);
        continue;
      }
      operate_bb(me,to,GOLD);
      return make_drop_move(to,GOLD_H);
    }
  }
  ASSERT(is_ok());
  if(hand.is_have(SILVER_H)){
    if(have_bishop && have_gold){
      //銀で積むことはない
      ;
    }else{
      if(have_bishop && !have_gold){
        //前だけチェック
        target = get_target<PAWN>(me,opp_king_sq);
      }else if(!have_bishop && have_gold){
        //斜め後ろ王手だけチェック
        target = (~get_target<GOLD>(me,opp_king_sq)) & get_target<SILVER>(me,opp_king_sq);
      }else if(!have_bishop && !have_gold){
        //銀の利きすべてチェック
        target = get_target<SILVER>(me,opp_king_sq);
      }else{
        ASSERT(false);
      }
      target &= drop_target;
      while(target.is_not_zero()){
        const auto to = target.lsb<D>();
        if(!attack_to(me,to).is_not_zero()){
          continue;
        }
        operate_bb(me,to,SILVER);
        if(can_escape(opp,opp_king_sq,to,false)){
          operate_bb(me,to,SILVER);
          continue;
        }
        operate_bb(me,to,SILVER);
        return make_drop_move(to,SILVER_H);
      }

    }
  }
  ASSERT(is_ok());
  if(hand.is_have(KNIGHT_H)){
    target = get_target<KNIGHT>(me,opp_king_sq) & drop_target;
    while(target.is_not_zero()){
      const auto to = target.lsb<D>();
      //桂馬は利きがなくていい
      //if(!attack_to(me,to).is_not_zero()){
      //  continue;
      //}
      operate_bb(me,to,KNIGHT);
      if(can_escape(opp,opp_king_sq,to,false)){
        operate_bb(me,to,KNIGHT);
        continue;
      }
      operate_bb(me,to,KNIGHT);
      return make_drop_move(to,KNIGHT_H);
    }
  }
  ASSERT(is_ok());

  //move
  //const auto pins = pinned_piece(me);
  const auto pins = ci.pinned();
  ASSERT(pins == pinned_piece(me));
  //prook
  Bitboard piece = piece_bb(me,PROOK);
  while(piece.is_not_zero()){
    const auto from = piece.lsb<D>();
    auto target = attack_from<PROOK>(me,from)
                & get_target<PROOK>(me,opp_king_sq)
                & (~color_bb(me));
    if(!target.is_not_zero()){
      continue;
    }
    operate_bb(me,from,PROOK);
    do{
      const auto to = target.lsb<D>();
      if(!pseudo_is_legal(from,to,pins)){
        continue;
      }
      operate_bb(me,to,PROOK);
      if(!attack_to(me,to).is_not_zero()){
        operate_bb(me,to,PROOK);
        continue;
      }
      if(can_escape(opp,opp_king_sq,to,move_is_discover(from,to,ci.discover_checker()))){
        operate_bb(me,to,PROOK);
        continue;
      }
      operate_bb(me,to,PROOK);
      operate_bb(me,from,PROOK);
      return make_move(from,to);
    }while(target.is_not_zero());
    operate_bb(me,from,PROOK);
  }
  ASSERT(is_ok());
  //pbishop
  piece = piece_bb(me,PBISHOP);
  while(piece.is_not_zero()){
    const auto from = piece.lsb<D>();
    auto target = attack_from<PBISHOP>(me,from)
                & get_target<PBISHOP>(me,opp_king_sq)
                & (~color_bb(me));
    if(!target.is_not_zero()){
      continue;
    }
    operate_bb(me,from,PBISHOP);
    do{
      const auto to = target.lsb<D>();
      if(!pseudo_is_legal(from,to,pins)){
        continue;
      }
      operate_bb(me,to,PBISHOP);
      if(!attack_to(me,to).is_not_zero()){
        operate_bb(me,to,PBISHOP);
        continue;
      }
      if(can_escape(opp,opp_king_sq,to,move_is_discover(from,to,ci.discover_checker()))){
        operate_bb(me,to,PBISHOP);
        continue;
      }
      operate_bb(me,to,PBISHOP);
      operate_bb(me,from,PBISHOP);
      return make_move(from,to);
    }while(target.is_not_zero());
    operate_bb(me,from,PBISHOP);
  }
  ASSERT(is_ok());
  //gold
  piece = golds_bb(me) & get_gold_check_table(me,opp_king_sq);
  while(piece.is_not_zero()){
    const auto from = piece.lsb<D>();
    auto target = attack_from<GOLD>(me,from)
                & get_target<GOLD>(me,opp_king_sq)
                & (~color_bb(me));
    const auto pt = piece_to_piece_type(square(from));
    if(!target.is_not_zero()){
      continue;
    }
    operate_bb(me,from,pt);
    do{
      const auto to = target.lsb<D>();
      if(!pseudo_is_legal(from,to,pins)){
        continue;
      }
      operate_bb(me,to,pt);
      if(!attack_to(me,to).is_not_zero()){
        operate_bb(me,to,pt);
        continue;
      }
      if(can_escape(opp,opp_king_sq,to,move_is_discover(from,to,ci.discover_checker()))){
        operate_bb(me,to,pt);
        continue;
      }
      operate_bb(me,to,pt);
      operate_bb(me,from,pt);
      return make_move(from,to);
    }while(target.is_not_zero());
    operate_bb(me,from,pt);
  }
  ASSERT(is_ok());
  //rook 1-3
  piece = piece_bb(me,ROOK) & get_prom_area(me);
  while(piece.is_not_zero()){
    const auto from = piece.lsb<D>();
    auto target = attack_from<ROOK>(me,from)
                & get_target<PROOK>(me,opp_king_sq)
                & (~color_bb(me));
    if(!target.is_not_zero()){
      continue;
    }
    operate_bb(me,from,ROOK);
    do{
      const auto to = target.lsb<D>();
      if(!pseudo_is_legal(from,to,pins)){
        continue;
      }
      operate_bb(me,to,PROOK);
      if(!attack_to(me,to).is_not_zero()){
        operate_bb(me,to,PROOK);
        continue;
      }
      if(can_escape(opp,opp_king_sq,to,move_is_discover(from,to,ci.discover_checker()))){
        operate_bb(me,to,PROOK);
        continue;
      }
      operate_bb(me,to,PROOK);
      operate_bb(me,from,ROOK);
      return make_prom_move(from,to);
    }while(target.is_not_zero());
    operate_bb(me,from,ROOK);
  }
  ASSERT(is_ok());
  //rook 4-9
  piece = piece_bb(me,ROOK) & get_not_prom_area(me);
  while(piece.is_not_zero()){
    const auto from = piece.lsb<D>();
    auto target = attack_from<ROOK>(me,from)
              & (~color_bb(me))
              & (   (get_prom_area(me)     & get_target<PROOK>(me,opp_king_sq))
                  | (get_not_prom_area(me) & get_target< ROOK>(me,opp_king_sq))
                );
    if(!target.is_not_zero()){
      continue;
    }
    operate_bb(me,from,ROOK);
    do{
      const auto to = target.lsb<D>();
      if(!pseudo_is_legal(from,to,pins)){
        continue;
      }
      (is_prom_square<me>(to)) ? operate_bb(me,to,PROOK)
                               : operate_bb(me,to,ROOK);
      if(!attack_to(me,to).is_not_zero()){
        (is_prom_square<me>(to)) ? operate_bb(me,to,PROOK)
                                 : operate_bb(me,to,ROOK);
        continue;
      }
      if(can_escape(opp,opp_king_sq,to,move_is_discover(from,to,ci.discover_checker()))){
        (is_prom_square<me>(to)) ? operate_bb(me,to,PROOK)
                                 : operate_bb(me,to,ROOK);
        continue;
      }
      (is_prom_square<me>(to)) ? operate_bb(me,to,PROOK)
                               : operate_bb(me,to,ROOK);
                                 operate_bb(me,from,ROOK);
      return (is_prom_square<me>(to)) ? make_prom_move(from,to)
                                      : make_move(from,to);
    }while(target.is_not_zero());
    operate_bb(me,from,ROOK);
  }
  ASSERT(is_ok());
  //bishop 1-3
  piece = piece_bb(me,BISHOP) & get_prom_area(me);
  while(piece.is_not_zero()){
    const auto from = piece.lsb<D>();
    auto target = attack_from<BISHOP>(me,from)
                & get_target<PBISHOP>(me,opp_king_sq)
                & (~color_bb(me));
    if(!target.is_not_zero()){
      continue;
    }
    operate_bb(me,from,BISHOP);
    do{
      const auto to = target.lsb<D>();
      if(!pseudo_is_legal(from,to,pins)){
        continue;
      }
      operate_bb(me,to,PBISHOP);
      if(!attack_to(me,to).is_not_zero()){
        operate_bb(me,to,PBISHOP);
        continue;
      }
      if(can_escape(opp,opp_king_sq,to,move_is_discover(from,to,ci.discover_checker()))){
        operate_bb(me,to,PBISHOP);
        continue;
      }
      operate_bb(me,to,PBISHOP);
      operate_bb(me,from,BISHOP);
      return make_prom_move(from,to);
    }while(target.is_not_zero());
    operate_bb(me,from,BISHOP);
  }
  ASSERT(is_ok());
  //bishop 4-9
  piece = piece_bb(me,BISHOP) & get_not_prom_area(me);
  while(piece.is_not_zero()){
    const auto from = piece.lsb<D>();
    auto target = attack_from<BISHOP>(me,from)
              & (~color_bb(me))
              & (   (get_prom_area(me)     & get_target<PBISHOP>(me,opp_king_sq))
                  | (get_not_prom_area(me) & get_target< BISHOP>(me,opp_king_sq))
              );
    if(!target.is_not_zero()){
      continue;
    }
    operate_bb(me,from,BISHOP);
    do{
      const auto to = target.lsb<D>();
      if(!pseudo_is_legal(from,to,pins)){
        continue;
      }
      (is_prom_square<me>(to)) ? operate_bb(me,to,PBISHOP)
                               : operate_bb(me,to,BISHOP);
      if(!attack_to(me,to).is_not_zero()){
        (is_prom_square<me>(to)) ? operate_bb(me,to,PBISHOP)
                                 : operate_bb(me,to,BISHOP);
        continue;
      }
      if(can_escape(opp,opp_king_sq,to,move_is_discover(from,to,ci.discover_checker()))){
        (is_prom_square<me>(to)) ? operate_bb(me,to,PBISHOP)
                                 : operate_bb(me,to,BISHOP);
        continue;
      }
      (is_prom_square<me>(to)) ? operate_bb(me,to,PBISHOP)
                               : operate_bb(me,to,BISHOP);
      operate_bb(me,from,BISHOP);
      return (is_prom_square<me>(to)) ? make_prom_move(from,to)
                                      : make_move(from,to);
    }while(target.is_not_zero());
    operate_bb(me,from,BISHOP);
  }
  ASSERT(is_ok());
  //silver (prom)
  piece = piece_bb(me,SILVER)
          & get_silver_check_table(me,opp_king_sq)
          & (get_prom_area(me) | get_pre_prom_area(me));

  while(piece.is_not_zero()){
    const auto from = piece.lsb<D>();
    auto target = attack_from<SILVER>(me,from)
                & get_target<GOLD>(me,opp_king_sq)
                & (~color_bb(me));
    if(!target.is_not_zero()){
      continue;
    }
    operate_bb(me,from,SILVER);
    do{
      const auto to = target.lsb<D>();
      if(!pseudo_is_legal(from,to,pins)){
        continue;
      }
      if(!is_prom_square<me>(from) && !is_prom_square<me>(to)){
        continue;
      }
      operate_bb(me,to,PSILVER);
      if(!attack_to(me,to).is_not_zero()){
        operate_bb(me,to,PSILVER);
        continue;
      }
      if(can_escape(opp,opp_king_sq,to,move_is_discover(from,to,ci.discover_checker()))){
        operate_bb(me,to,PSILVER);
        continue;
      }
      operate_bb(me,to,PSILVER);
      operate_bb(me,from,SILVER);
      return make_prom_move(from,to);
    }while(target.is_not_zero());
    operate_bb(me,from,SILVER);
  }
  ASSERT(is_ok());
  //silver (not prom)
  piece = piece_bb(me,SILVER) & get_silver_check_table(me,opp_king_sq);
  while(piece.is_not_zero()){
    const auto from = piece.lsb<D>();
    auto target = attack_from<SILVER>(me,from)
                & get_target<SILVER>(me,opp_king_sq)
                & (~color_bb(me));
    if(!target.is_not_zero()){
      continue;
    }
    operate_bb(me,from,SILVER);
    do{
      const auto to = target.lsb<D>();
      if(!pseudo_is_legal(from,to,pins)){
        continue;
      }
      operate_bb(me,to,SILVER);
      if(!attack_to(me,to).is_not_zero()){
        operate_bb(me,to,SILVER);
        continue;
      }
      if(can_escape(opp,opp_king_sq,to,move_is_discover(from,to,ci.discover_checker()))){
        operate_bb(me,to,SILVER);
        continue;
      }
      operate_bb(me,to,SILVER);
      operate_bb(me,from,SILVER);
      return make_move(from,to);
    }while(target.is_not_zero());
    operate_bb(me,from,SILVER);
  }
  ASSERT(is_ok());
  //knight prom
  piece = piece_bb(me,KNIGHT)
          & get_knight_check_table(me,opp_king_sq)
          & (get_prom_area(me) | get_pre_prom_area(me) | get_middle_area());
  while(piece.is_not_zero()){
    const auto from = piece.lsb<D>();
    auto target = attack_from<KNIGHT>(me,from)
                & get_target<GOLD>(me,opp_king_sq)
                & (~color_bb(me));
    if(!target.is_not_zero()){
      continue;
    }
    operate_bb(me,from,KNIGHT);
    do{
      const auto to = target.lsb<D>();
      if(!pseudo_is_legal(from,to,pins)){
        continue;
      }
      operate_bb(me,to,PKNIGHT);
      if(!attack_to(me,to).is_not_zero()){
        operate_bb(me,to,PKNIGHT);
        continue;
      }
      if(can_escape(opp,opp_king_sq,to,move_is_discover(from,to,ci.discover_checker()))){
        operate_bb(me,to,PKNIGHT);
        continue;
      }
      operate_bb(me,to,PKNIGHT);
      operate_bb(me,from,KNIGHT);
      return make_prom_move(from,to);
    }while(target.is_not_zero());
    operate_bb(me,from,KNIGHT);
  }
  ASSERT(is_ok());
  //knight (not prom)
  piece = piece_bb(me,KNIGHT)
          & get_knight_check_table(me,opp_king_sq)
          & (get_not_prom_area(me) & (~get_pre_prom_area(me)));
  while(piece.is_not_zero()){
    const auto from = piece.lsb<D>();
    auto target = attack_from<KNIGHT>(me,from)
                & get_target<KNIGHT>(me,opp_king_sq)
                & (~color_bb(me));
    if(!target.is_not_zero()){
      continue;
    }
    operate_bb(me,from,KNIGHT);
    do{
      const auto to = target.lsb<D>();
      if(!pseudo_is_legal(from,to,pins)){
        continue;
      }
      operate_bb(me,to,KNIGHT);
      if(!attack_to(me,to).is_not_zero()){
        operate_bb(me,to,KNIGHT);
        continue;
      }
      if(can_escape(opp,opp_king_sq,to,move_is_discover(from,to,ci.discover_checker()))){
        operate_bb(me,to,KNIGHT);
        continue;
      }
      operate_bb(me,to,KNIGHT);
      operate_bb(me,from,KNIGHT);
      return make_move(from,to);
    }while(target.is_not_zero());
    operate_bb(me,from,KNIGHT);
  }
  ASSERT(is_ok());
  //lance
  //TODO separate left and right
  piece = piece_bb(me,LANCE) & get_lance_check_table(me,opp_king_sq);
  while(piece.is_not_zero()){
    const auto from = piece.lsb<D>();
    auto target = attack_from<LANCE>(me,from)
                 & (~color_bb(me))
                 & (   (get_prom_area(me)     & get_target< GOLD>(me,opp_king_sq))
                     | (get_not_prom_area(me) & get_target<LANCE>(me,opp_king_sq))
                   );
    if(!target.is_not_zero()){
      continue;
    }
    operate_bb(me,from,LANCE);
    do{
      const auto to = target.lsb<D>();
      if(!pseudo_is_legal(from,to,pins)){
        continue;
      }
      (is_prom_square<me>(to)) ? operate_bb(me,to,PLANCE)
                               : operate_bb(me,to,LANCE);
      if(!attack_to(me,to).is_not_zero()){
        (is_prom_square<me>(to)) ? operate_bb(me,to,PLANCE)
                                 : operate_bb(me,to,LANCE);
        continue;
      }
      if(can_escape(opp,opp_king_sq,to,move_is_discover(from,to,ci.discover_checker()))){
        (is_prom_square<me>(to)) ? operate_bb(me,to,PLANCE)
                                 : operate_bb(me,to,LANCE);
        continue;
      }
      (is_prom_square<me>(to)) ? operate_bb(me,to,PLANCE)
                               : operate_bb(me,to,LANCE);
      operate_bb(me,from,LANCE);
      return (is_prom_square<me>(to)) ? make_prom_move(from,to)
                                      : make_move(from,to);
    }while(target.is_not_zero());
    operate_bb(me,from,LANCE);
  }
  ASSERT(is_ok());
  //pawn
  piece = piece_bb(me,PAWN) & get_pawn_check_table(me,opp_king_sq);
  while(piece.is_not_zero()){
    const auto from = piece.lsb<D>();
    auto target = attack_from<PAWN>(me,from)
                 & (~color_bb(me))
                 & (   (get_prom_area(me)     & get_target<GOLD>(me,opp_king_sq))
                     | (get_not_prom_area(me) & get_target<PAWN>(me,opp_king_sq))
                 );
    if(!target.is_not_zero()){
      continue;
    }
    operate_bb(me,from,PAWN);
    do{
      const auto to = target.lsb<D>();
      if(!pseudo_is_legal(from,to,pins)){
        continue;
      }
      (is_prom_square<me>(to)) ? operate_bb(me,to,PPAWN)
                               : operate_bb(me,to,PAWN);
      if(!attack_to(me,to).is_not_zero()){
        (is_prom_square<me>(to)) ? operate_bb(me,to,PPAWN)
                                 : operate_bb(me,to,PAWN);
        continue;
      }
      if(can_escape(opp,opp_king_sq,to,move_is_discover(from,to,ci.discover_checker()))){
        (is_prom_square<me>(to)) ? operate_bb(me,to,PPAWN)
                                 : operate_bb(me,to,PAWN);
        continue;
      }
      (is_prom_square<me>(to)) ? operate_bb(me,to,PPAWN)
                               : operate_bb(me,to,PAWN);
      operate_bb(me,from,PAWN);
      return (is_prom_square<me>(to)) ? make_prom_move(from,to)
                                      : make_move(from,to);
    }while(target.is_not_zero());
    operate_bb(me,from,PAWN);
  }
  ASSERT(is_ok());
  return move_none();
}
void Position::operate_bb(const Color c, const Square sq, const PieceType pt){

  color_bb_[c].Xor(sq);
  piece_bb_[pt].Xor(sq);
  piece_bb_[OCCUPIED].Xor(sq);
}
template<PieceType pt> Bitboard get_target(const Color c, const Square ksq){

  if(false){
  }else if(pt == PROOK || pt == PBISHOP){
    return get_pseudo_attack<KING>(opp_color(c),ksq);
  }else if(pt == ROOK){
    return get_pseudo_attack<GOLD>(opp_color(c),ksq)
         & get_pseudo_attack<GOLD>(c,ksq);
  }else if(pt == BISHOP){
    return get_pseudo_attack<SILVER>(opp_color(c),ksq)
         & get_pseudo_attack<SILVER>(c,ksq);
  }else if(pt == LANCE){
    return get_pseudo_attack<PAWN>(opp_color(c),ksq);
  }else{
    return get_pseudo_attack<pt>(opp_color(c),ksq);
  }
}
bool Position::can_escape(const Color c,
                          const Square ksq,
                          const Square checker,
                          const bool is_double_check){

   //王手している駒は利きが利いているので、玉で取れない
  //escape king
  operate_bb(c,ksq,KING);
  Bitboard target = get_king_attack(ksq) & (~color_bb(c));
  //TODO improve here
  ASSERT(square_is_ok(checker));
  const auto to_piece = square(checker);
  //これがないとおかしなことになるのでしょうがない
  if(to_piece){
    operate_bb(c,checker,piece_to_piece_type(to_piece));
  }
  //玉で取れるか？
  while(target.is_not_zero()){
    ASSERT(target.is_not_zero());
    const auto to = target.lsb<D>();
    operate_bb(c,to,KING);
    if(!attack_to(opp_color(c),to).is_not_zero()){
      operate_bb(c,to,KING);
      operate_bb(c,ksq,KING);
      if(to_piece){
        operate_bb(c,checker,piece_to_piece_type(to_piece));
      }
      return true;
    }
    operate_bb(c,to,KING);
  }
  operate_bb(c,ksq,KING);

  //両王手の場合は玉が逃げるしかない
  if(!is_double_check){
    Bitboard cap = attack_to(c,checker);
    const Bitboard pins = pinned_piece(c);
    //capture checker by other piece
    while(cap.is_not_zero()){
      ASSERT(cap.is_not_zero());
      const auto from = cap.lsb<D>();
      if(pseudo_is_legal(c,from,checker,pins)){
        if(to_piece){
          operate_bb(c,checker,piece_to_piece_type(to_piece));
        }
        return true;
      }
    }
  }
  if(to_piece){
    operate_bb(c,checker,piece_to_piece_type(to_piece));
  }
  return false;
}
bool Position::mate_one_is_ok(const Move & move){

  ASSERT(is_ok());
  CheckInfo ci(*this);

  bool check = move_is_check(move,ci);
  if(!check){
    std::cout<<"move is not check\n";
    print();
    std::cout<<move.str(*this)<<std::endl;
    return false;
  }
  do_move(move,ci,check);
  int num = move_num_debug(false);
  undo_move(move);
  if(num){
    std::cout<<"not mate move\n";
    print();
    std::cout<<move.str(*this)<<std::endl;
    return false;
  }
  return true;
}
template Move Position::mate_one<BLACK>(CheckInfo & ci);
template Move Position::mate_one<WHITE>(CheckInfo & ci);

