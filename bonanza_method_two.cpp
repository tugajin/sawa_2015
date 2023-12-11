#include "myboost.h"
#include "type.h"
#include "bonanza_method.h"
#include "random.h"
#include "evaluator.h"
#include <iostream>
#include <float.h>
#include <utility>
#include <limits.h>
#include <string>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include "boost/foreach.hpp"
#include "boost/format.hpp"

static int record_num;

u32 PhaseTwo::worker(){
  init();
  record_num = 0;
  std::stringstream name;
  name<<"save_pv.pv";
  name<<thread_id_;
  read_pv_stream.open(name.str(),std::ios::binary);
  if(read_pv_stream.fail()){
    std::cout<<thread_id_<<"fail\n";
    return 0;
  }
  while(true){
    u32 ret = read_pv();
    if(ret == 0){
      break;
    }
    if(ret == 1){
      ret = calc_grad();
      position_num_++;
      record_num++;
    }else{
      std::cout<<"read pv error\n";
    }
  }
  read_pv_stream.close();
  return 1;
}
u32 PhaseTwo::calc_grad(){

  //std::cout<<"calc "<<thread_id_;
  Value record_value,value;
  record_value = -VALUE_INF;
  const auto root_turn = pos_.turn();
  int depth;
  double dT,sum_dT;
  sum_dT = 0.0;
  //兄弟局面学習
  //follow record pv
  for(depth = 0; pv_[0][depth].value() != 0; ++depth){
    CheckInfo ci(pos_);
    if(!pv_[0][depth].is_ok(pos_)){
        std::cout<<"rec "<<record_num<<" "<<pv_num_<<std::endl;
        pos_.print();
        std::cout<<"depth "<<depth<<std::endl;
        std::cout<<pv_[0][depth].value()<<std::endl;
        //BREAK_POINT;
        return 0;
    }
    pos_.do_move(pv_[0][depth],ci,pos_.move_is_check(pv_[0][depth],ci));
  }
  ASSERT(pos_.is_ok());
  value = evaluate(pos_);
  if(pos_.turn() != root_turn){
    value = -value;
  }
  record_value = value;
  for(--depth; depth>=0; --depth){
    pos_.undo_move(pv_[0][depth]);
  }
  //follow other pv
  for(int i = 1; i < pv_num_; ++i){
    for(depth = 0; pv_[i][depth].value() != 0; ++depth){
      CheckInfo ci(pos_);
      if(!pv_[i][depth].is_ok(pos_)){
        std::cout<<"fail follow other pv\n";
        std::cout<<pv_[i][depth].str()<<std::endl;
        pos_.print();
        //BREAK_POINT;
        return 0;
      }
      pos_.do_move(pv_[i][depth],ci,pos_.move_is_check(pv_[i][depth],ci));
    }
    value = evaluate(pos_);
    if(pos_.turn() != root_turn){
      value = -value;
    }
    target_ += func(value - record_value);
    dT = dfunc(value - record_value);
    if(root_turn == WHITE){
      dT = -dT;
    }
    sum_dT += dT;
    inc_grad(-dT);
    for(--depth; depth>=0; --depth){
      pos_.undo_move(pv_[i][depth]);
    }
  }
  //follow record pv
  for(depth = 0; pv_[0][depth].value() != 0; ++depth){
    CheckInfo ci(pos_);
    pos_.do_move(pv_[0][depth],ci,pos_.move_is_check(pv_[0][depth],ci));
  }
  inc_grad(sum_dT);
  for(--depth; depth>=0; --depth){
    pos_.undo_move(pv_[0][depth]);
  }

  //std::cout<<"cal end "<<thread_id_<<std::endl;
  return 1;
}
inline Kvalue calc_kvalue(float g, const Kvalue v){
  if( v > 0){
    g -= FV_PENALTY;
  }else if( v < 0){
    g += FV_PENALTY;
  }
  Value h = Value((random32() & 1) + (random32() & 1));
  Value ret = Value(v) + h *sign(g);
  if(ret >= SHRT_MAX){
    std::cout<<"fv.bin is out of bound\n";
  }else if(ret <= SHRT_MIN){
    std::cout<<"fv.bin is out of bound\n";
  }
  if(ret * Value(v) < 0){
    ret = VALUE_ZERO;
  }
  return Kvalue(ret);
}
u32 PhaseTwo::update_weight(){
  std::vector<std::pair<double,PieceType>>g(PIECE_TYPE_SIZE);
  double max_grad  = - DBL_MAX;
  double min_grad  =   DBL_MAX;
  for(PieceType pt = EMPTY_T; pt < PIECE_TYPE_SIZE; pt++){
    g[pt].first = grad_.piece_[pt];
    g[pt].second = pt;
    max_grad = std::max(max_grad,grad_.piece_[pt]);
    min_grad = std::min(min_grad,grad_.piece_[pt]);
  }
  g[KING].first = DBL_MAX;
  g[EMPTY].first = DBL_MAX;

  std::sort(g.begin(),g.end());

  //bonanza's random shuffle
  u32 u32rand = random32();
  u32 u       = u32rand % 7U;
      u32rand = u32rand / 7U;
  std::pair<double,PieceType>tmp;
  tmp = g[u+6]; g[u+6] = g[12]; g[12] = tmp;
  for(int i = 5; i > 0; i--){
    u       = u32rand % (i+1);
    u32rand = u32rand / (i+1);
    tmp = g[u]; g[u] = g[i]; g[i] = tmp;

    u       = u32rand % (i+1);
    u32rand = u32rand / (i+1);
    tmp = g[u+6]; g[u+6] = g[i+6]; g[i+6] = tmp;
  }

  p_value[g[ 0].second] += -2;
  p_value[g[ 1].second] += -2;
  p_value[g[ 2].second] += -1;
  p_value[g[ 3].second] += -1;
  p_value[g[ 4].second] += -1;
  p_value[g[ 5].second] +=  0;
  p_value[g[ 6].second] +=  0;
  p_value[g[ 7].second] +=  0;
  p_value[g[ 8].second] +=  1;
  p_value[g[ 9].second] +=  1;
  p_value[g[10].second] +=  1;
  p_value[g[11].second] +=  2;
  p_value[g[12].second] +=  2;

  set_piece_value();

  max_value_ = SHRT_MIN;
  min_value_ = SHRT_MAX;
#ifndef PAIR
  for(int i = 0; i < int(SQUARE_SIZE); i++){
    for(int j = 0; j < int(SQUARE_SIZE); j++){
      for(int k = 0; k <int(kkp_end); k++){
        akkp[i][j][k] = calc_kvalue(grad_.kkp_[i][j][k],akkp[i][j][k]);
        sum_fv_ += abs(akkp[i][j][k]);
        if(akkp[i][j][k]){
          not_zero_num_++;
          max_value_ = std::max(max_value_,akkp[i][j][k]);
          min_value_ = std::min(min_value_,akkp[i][j][k]);
        }
      }
    }
  }
#endif
  for(int i = 0; i < int(SQUARE_SIZE); i++){
    for(int j = 0; j < int(fe_end); j++){
      for(int k = 0; k <int(fe_end); k++){
#ifdef PAIR
        if(j != k){
          continue;
        }
#endif
        pc_on_sq[i][j][k] = calc_kvalue(grad_.pc_on_sq_[i][j][k],pc_on_sq[i][j][k]);
        sum_fv_ += abs(pc_on_sq[i][j][k]);
        if(pc_on_sq[i][j][k]){
          not_zero_num_++;
          max_value_ = std::max(max_value_,pc_on_sq[i][j][k]);
          min_value_ = std::min(min_value_,pc_on_sq[i][j][k]);
        }
      }
    }
  }
  return 1;
}
u32 PhaseTwo::read_pv(){

  std::string buf;
  getline(read_pv_stream,buf);
  if(buf.empty()){
    return 0;
  }
  typedef boost::tokenizer<> tokenizer1;
  tokenizer1 tok1(buf);
  int sp = 0;
  int len = 0;
  Color c = COLOR_ILLEGAL;
  Hand bh,wh;
  pv_num_ = 0;
  for(tokenizer1::iterator it = tok1.begin(); it != tok1.end(); ++it){
    int v = boost::lexical_cast<int>(*it);
    if(sp < 81){
      ar_[bb_to_array(sp)] = Piece(v);
      sp++;
    }else if(sp == 81){
      c = Color(v);
      sp++;
    }else if(sp == 82){
      bh.set(v);
      sp++;
    }else if(sp == 83){
      wh.set(v);
      sp++;
    }else{
      pv_[pv_num_][len++].set(v);
      if(v == 0){
        pv_num_++;
        len = 0;
      }
    }
  }
  if(sp <= 83){
    return -1;
  }
  if(!pos_.init_position(ar_,c,bh,wh)){
    return -1;
  }
  if(pv_num_ == 0){
    std::cout<<"pv num == 0\n";
    return -1;
  }

  return 1;
}
void PhaseTwo::init_info(const int num){
  thread_id_ = num;
}
void PhaseTwo::print(){
  std::cout<<" Target : "<<double(target_/double(position_num_))
           <<" Target : "<<double(target_)
           <<" Position num : "<<position_num_
           <<" sum_fv : "<<sum_fv_
           <<" not zero : "<<not_zero_num_
           <<" average : "<<sum_fv_/not_zero_num_
           <<" max : "<<max_value_
           <<" min : "<<min_value_<<std::endl;
  g_write_of_stream<<double(target_/double(position_num_))<<","
                   <<double(target_)<<",";
  g_write_of_stream<<"\n";
  g_write_of_stream.flush();
}
void PhaseTwo::init(){
  target_ = 0.0;
  sum_fv_ = 0;
  not_zero_num_ = 0;
  position_num_ = 0;
  PIECE_TYPE_FOREACH(p){
    piece_feat_[p] = 0;
    grad_.piece_[p] = 0.0;
  }
  for(int i = 0; i < int(SQUARE_SIZE); i++){
    for(int j = 0; j < int(SQUARE_SIZE); j++){
      for(int k = 0; k <int(kkp_end); k++){
        grad_.kkp_[i][j][k] = 0;
      }
    }
  }
  for(int i = 0; i < int(SQUARE_SIZE); i++){
    for(int j = 0; j < int(fe_end); j++){
      for(int k = 0; k <int(fe_end); k++){
        grad_.pc_on_sq_[i][j][k] = 0;
      }
    }
  }
}
void PhaseTwo::inc_grad(const double dinc){
  const float f = (float)(dinc / (double)FV_SCALE);
  u32 nlist = 0;
  EvalIndex list0[52],list1[52];
  const auto bking = bb_to_array(pos_.king_sq(BLACK));
  const auto wking = opp_square(bb_to_array(pos_.king_sq(WHITE)));
  Value x = VALUE_ZERO;
  nlist = make_list(list0,list1,f,x);
  PIECE_TYPE_FOREACH(p){
    grad_.piece_[p] += dinc * (double)piece_feat_[p];
    //std::cout<<str_piece_type_name(p)<<" "<<dinc * (double)piece_feat_[p]<<std::endl;
  }
  for(u32 i = 0; i < nlist; ++i){
    const EvalIndex k0 = list0[i];
    const EvalIndex k1 = list1[i];
    for(u32 j = 0; j <= i; ++j){
      const EvalIndex l0 = list0[j];
      const EvalIndex l1 = list1[j];
      ASSERT(int(k0) >= 0);
      ASSERT(int(l0) >= 0);
      ASSERT(int(k1) >= 0);
      ASSERT(int(l1) >= 0);
      ASSERT(k0 <= fe_end);
      ASSERT(l0 <= fe_end);
      ASSERT(k1 <= fe_end);
      ASSERT(l1 <= fe_end);
      grad_.pc_on_sq_[bking][k0][l0] += f;
      grad_.pc_on_sq_[wking][k1][l1] -= f;
      x += pc_pc_on_sq(bking,k0,l0);
      x -= pc_pc_on_sq(wking,k1,l1);
    }
  }
  x += pos_.material() * FV_SCALE;
  x /= FV_SCALE;
}
u32 PhaseTwo::make_list(EvalIndex list0[52],
              EvalIndex list1[52],
              const float f,
              Value & v){
  u32 nlist = 14;
  const auto bking_org = pos_.king_sq(BLACK);
  const auto wking_org = pos_.king_sq(WHITE);
  const auto bking = bb_to_array(bking_org);
  const auto wking = bb_to_array(wking_org);
  const auto bking2 = opp_square(bking);
  const auto wking2 = opp_square(wking);
  const auto hand_b = pos_.hand(BLACK);
  const auto hand_w = pos_.hand(WHITE);
  Bitboard occ_b = pos_.color_bb(BLACK);
  Bitboard occ_w = pos_.color_bb(WHITE);
  occ_b.Xor(bking_org);//delete king
  occ_w.Xor(wking_org);//delete king
  Square sq;

  for(PieceType pt = EMPTY_T; pt < PIECE_TYPE_SIZE; pt++){
    piece_feat_[pt] = 0;
  }
  HAND_FOREACH(hp){
    const auto p = hand_piece_to_piece_type(hp);
    piece_feat_[p] = pos_.hand(BLACK).num(hp)
                   - pos_.hand(WHITE).num(hp);
  }

  list0[ 0] = f_hand_pawn   + EvalIndex(hand_b.num(PAWN_H));
  list0[ 1] = e_hand_pawn   + EvalIndex(hand_w.num(PAWN_H));
  list0[ 2] = f_hand_lance  + EvalIndex(hand_b.num(LANCE_H));
  list0[ 3] = e_hand_lance  + EvalIndex(hand_w.num(LANCE_H));
  list0[ 4] = f_hand_knight + EvalIndex(hand_b.num(KNIGHT_H));
  list0[ 5] = e_hand_knight + EvalIndex(hand_w.num(KNIGHT_H));
  list0[ 6] = f_hand_silver + EvalIndex(hand_b.num(SILVER_H));
  list0[ 7] = e_hand_silver + EvalIndex(hand_w.num(SILVER_H));
  list0[ 8] = f_hand_gold   + EvalIndex(hand_b.num(GOLD_H));
  list0[ 9] = e_hand_gold   + EvalIndex(hand_w.num(GOLD_H));
  list0[10] = f_hand_bishop + EvalIndex(hand_b.num(BISHOP_H));
  list0[11] = e_hand_bishop + EvalIndex(hand_w.num(BISHOP_H));
  list0[12] = f_hand_rook   + EvalIndex(hand_b.num(ROOK_H));
  list0[13] = e_hand_rook   + EvalIndex(hand_w.num(ROOK_H));

  list1[ 0] = f_hand_pawn   + EvalIndex(hand_w.num(PAWN_H));
  list1[ 1] = e_hand_pawn   + EvalIndex(hand_b.num(PAWN_H));
  list1[ 2] = f_hand_lance  + EvalIndex(hand_w.num(LANCE_H));
  list1[ 3] = e_hand_lance  + EvalIndex(hand_b.num(LANCE_H));
  list1[ 4] = f_hand_knight + EvalIndex(hand_w.num(KNIGHT_H));
  list1[ 5] = e_hand_knight + EvalIndex(hand_b.num(KNIGHT_H));
  list1[ 6] = f_hand_silver + EvalIndex(hand_w.num(SILVER_H));
  list1[ 7] = e_hand_silver + EvalIndex(hand_b.num(SILVER_H));
  list1[ 8] = f_hand_gold   + EvalIndex(hand_w.num(GOLD_H));
  list1[ 9] = e_hand_gold   + EvalIndex(hand_b.num(GOLD_H));
  list1[10] = f_hand_bishop + EvalIndex(hand_w.num(BISHOP_H));
  list1[11] = e_hand_bishop + EvalIndex(hand_b.num(BISHOP_H));
  list1[12] = f_hand_rook   + EvalIndex(hand_w.num(ROOK_H));
  list1[13] = e_hand_rook   + EvalIndex(hand_b.num(ROOK_H));

  grad_.kkp_[bking ][wking ][kkp_hand_pawn   + hand_b.num(PAWN_H)]   += f;
  grad_.kkp_[bking ][wking ][kkp_hand_lance  + hand_b.num(LANCE_H)]  += f;
  grad_.kkp_[bking ][wking ][kkp_hand_knight + hand_b.num(KNIGHT_H)] += f;
  grad_.kkp_[bking ][wking ][kkp_hand_silver + hand_b.num(SILVER_H)] += f;
  grad_.kkp_[bking ][wking ][kkp_hand_gold   + hand_b.num(GOLD_H)]   += f;
  grad_.kkp_[bking ][wking ][kkp_hand_bishop + hand_b.num(BISHOP_H)] += f;
  grad_.kkp_[bking ][wking ][kkp_hand_rook   + hand_b.num(ROOK_H)]   += f;

  grad_.kkp_[wking2][bking2][kkp_hand_pawn   + hand_w.num(PAWN_H)]   -= f;
  grad_.kkp_[wking2][bking2][kkp_hand_lance  + hand_w.num(LANCE_H)]  -= f;
  grad_.kkp_[wking2][bking2][kkp_hand_knight + hand_w.num(KNIGHT_H)] -= f;
  grad_.kkp_[wking2][bking2][kkp_hand_silver + hand_w.num(SILVER_H)] -= f;
  grad_.kkp_[wking2][bking2][kkp_hand_gold   + hand_w.num(GOLD_H)]   -= f;
  grad_.kkp_[wking2][bking2][kkp_hand_bishop + hand_w.num(BISHOP_H)] -= f;
  grad_.kkp_[wking2][bking2][kkp_hand_rook   + hand_w.num(ROOK_H)]   -= f;

  v += kkp(bking, wking, kkp_hand_pawn   + hand_b.num(PAWN_H));
  v += kkp(bking, wking, kkp_hand_lance  + hand_b.num(LANCE_H));
  v += kkp(bking, wking, kkp_hand_knight + hand_b.num(KNIGHT_H));
  v += kkp(bking, wking, kkp_hand_silver + hand_b.num(SILVER_H));
  v += kkp(bking, wking, kkp_hand_gold   + hand_b.num(GOLD_H));
  v += kkp(bking, wking, kkp_hand_bishop + hand_b.num(BISHOP_H));
  v += kkp(bking, wking, kkp_hand_rook   + hand_b.num(ROOK_H));

  v -= kkp(wking2, bking2, kkp_hand_pawn   + hand_w.num(PAWN_H));
  v -= kkp(wking2, bking2, kkp_hand_lance  + hand_w.num(LANCE_H));
  v -= kkp(wking2, bking2, kkp_hand_knight + hand_w.num(KNIGHT_H));
  v -= kkp(wking2, bking2, kkp_hand_silver + hand_w.num(SILVER_H));
  v -= kkp(wking2, bking2, kkp_hand_gold   + hand_w.num(GOLD_H));
  v -= kkp(wking2, bking2, kkp_hand_bishop + hand_w.num(BISHOP_H));
  v -= kkp(wking2, bking2, kkp_hand_rook   + hand_w.num(ROOK_H));

  BB_FOR_EACH(occ_b,sq,
      const auto p = pos_.square(sq);
      const auto pt = piece_to_piece_type(p);
      ++piece_feat_[pt];
      ASSERT(pt != KING);
      sq = bb_to_array(sq);
      list0[nlist] = pc_on_sq_index[p] + EvalIndex(sq);
      list1[nlist] = pc_on_sq_index[opp_piece(p)] + EvalIndex(opp_square(sq));
      grad_.kkp_[bking][wking][kkp_index[p] + EvalIndex(sq)] += f;
      v += kkp(bking,wking,kkp_index[p] + EvalIndex(sq));
      ++nlist;
      ASSERT(nlist < 52);
      );
  BB_FOR_EACH(occ_w,sq,
      const auto p = pos_.square(sq);
      const auto pt = piece_to_piece_type(p);
      --piece_feat_[pt];
      ASSERT(pt != KING);
      sq = bb_to_array(sq);
      list0[nlist] = pc_on_sq_index[p] + EvalIndex(sq);
      list1[nlist] = pc_on_sq_index[opp_piece(p)] + EvalIndex(opp_square(sq));
      grad_.kkp_[wking2][bking2][kkp_index[opp_piece(p)] + EvalIndex(opp_square(sq))] -= f;
      v -= kkp(wking2,bking2,kkp_index[opp_piece(p)] + EvalIndex(opp_square(sq)));
      ++nlist;
      ASSERT(nlist < 52);
      );

  return nlist;
}

