#include "myboost.h"
#include "type.h"
#include "evaluator.h"
#include "position.h"
#include "search.h"
#include "search_info.h"
#include "random.h"
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <boost/format.hpp>
#include <boost/static_assert.hpp>

//#define PcPcOnSq(k,i,j)     pc_on_sq2[k][(i)*((i)+1)/2+(j)]
//#define PcPcOnSqAny(k,i,j) ( i >= j ? PcPcOnSq(k,i,j) : PcPcOnSq(k,j,i) )

Value p_value[PIECE_SIZE];
Value p_value_ex[PIECE_SIZE];
Value p_value_pm[PIECE_SIZE];
Kvalue pc_on_sq[SQUARE_SIZE][fe_end][fe_end];
Kvalue akkp[SQUARE_SIZE][SQUARE_SIZE][kkp_end];
//Kvalue pc_on_sq2[SQUARE_SIZE][pos_n];

static constexpr u64 EVAL_TRANS_SHIFT = 21;
static constexpr u64 EVAL_TRANS_SIZE = 1 << EVAL_TRANS_SHIFT;
static constexpr u64 EVAL_TRANS_MASK = EVAL_TRANS_SIZE - 1;

static constexpr Value value_offset_ = Value(32768);

constexpr Value nyugyoku_bounus[RANK_SIZE] = {
  Value(   0), Value(   0), Value(  50),
  Value( 210), Value( 420), Value( 730),
  Value(1040), Value(1350), Value(1660),
};

u64 eval_trans[EVAL_TRANS_SIZE];
                                                  //gold  pawn lance knight silver bishop rook
constexpr int pc_on_sq_hand_index[HAND_PIECE_SIZE] = {    8,    0,    2,     4,     6,    10,  12  };
constexpr int kkp_hand_index[HAND_PIECE_SIZE] = { kkp_hand_gold, kkp_hand_pawn, kkp_hand_lance, kkp_hand_knight,
                                              kkp_hand_silver, kkp_hand_bishop, kkp_hand_rook };

template<Color c, bool last_move_is_tail>u32 make_list(Position & pos, EvalIndex list0[],
                                              EvalIndex list1[], Value & value);

template<Color c>bool calc_diff(Position & pos,Value & value,
                                EvalIndex list0[], EvalIndex list1[]);
template<Color piece_col>Value doapc(Position & pos, const Piece pc, const Square xy,
                                EvalIndex list0[], EvalIndex list1[], const u32 nlist);

template<Color piece_col>Value doacapt(Position & pos, const HandPiece pc, const int hand_num,
                               EvalIndex list0[], EvalIndex list1[], const u32 nlist);
void convert_fv();
Value evaluate_nyugyoku(Position & pos);

void init_eval_trans(){

  std::cout<<"cleaning eval trans...";
  for(u64 i = 0; i < EVAL_TRANS_SIZE; i++){
    eval_trans[i] = 0;
  }
  std::cout<<"end\n";
}

static inline void store(const Key & key, const Hand & hand_b, const Value value){

#ifdef LEARN
  return;
#endif

  u64 entry = (key ^ (Key(hand_b.value()) << EVAL_TRANS_SHIFT));
  u64 index = entry & EVAL_TRANS_MASK;
  ASSERT(index < EVAL_TRANS_SIZE);
  entry = entry & (~EVAL_TRANS_MASK);
  const Value store_value = value + (FV_SCALE * value_offset_);
  entry = entry | u64(store_value);
  eval_trans[index] = entry;

}
static inline bool probe(const Key & key, const Hand & hand_b, Value & value){

#ifdef LEARN
  return false;
#endif

  u64 entry = (key ^ (Key(hand_b.value()) << EVAL_TRANS_SHIFT));
  u64 index = entry & EVAL_TRANS_MASK;
  ASSERT(index < EVAL_TRANS_SIZE);
  entry = entry & (~EVAL_TRANS_MASK);
  u64 word = eval_trans[index];
  if((word & (~EVAL_TRANS_MASK)) == entry){
    Value sv = Value(word & EVAL_TRANS_MASK);
    sv -= FV_SCALE * value_offset_;
    value = sv;
    return true;
  }
  return false;
}

void set_piece_value(){

  for(Piece p = GOLD_B; p <= ROOK_B; p++){
    if(can_prom_piece(p)){
      const auto p2 = prom_piece(p);
      p_value_ex[p]  = p_value[p] + p_value[p ];
      p_value_ex[p2] = p_value[p] + p_value[p2];
      p_value_pm[p]  = p_value[p2] - p_value[p];
      p_value_pm[p2] = p_value[p2] - p_value[p];
    }else{
      p_value_ex[p]  = p_value[p] + p_value[p ];
    }
  }
  p_value_ex[KING_B] = p_value[KING];
  W_PIECE_FOREACH(p){
    const auto p2 = opp_piece(p);
    p_value[p]    = -p_value[p2];
    p_value_ex[p] = -p_value_ex[p2];
    p_value_pm[p] = -p_value_pm[p2];
  }
}
void init_evauator(){

  std::cout<<"init evaluate weight ...";

  std::memset(p_value, 0, sizeof(p_value));
  std::memset(p_value_ex, 0, sizeof(p_value_ex));
  std::memset(p_value_pm, 0, sizeof(p_value_pm));
  std::memset(pc_on_sq, 0, sizeof(pc_on_sq));
  std::memset(akkp, 0, sizeof(akkp));

  std::ifstream ifs("material.txt");

  if(ifs.fail()){
    std::cout<<"can't open material.txt\n";
    exit(1);
  }

  std::string buf;
  int sp = 1;
  Value sum = VALUE_ZERO;
  while(getline(ifs,buf)){
    p_value[sp] = Value(atoi(buf.c_str()));
    if(sp != int(KING)){
      sum += p_value[sp];
    }
    sp++;
    if(sp >= int(PIECE_SIZE)){
      std::cout<<"illegal material.txt\n";
      exit(1);
    }
  }
  if(p_value[KING_B] < sum){
    std::cout<<"king value is too small sum "<<sum<<std::endl;
    exit(1);
  }
  set_piece_value();

  FILE *pf;

  pf = fopen("fv.bin","rb");
  if(pf == nullptr){
    std::cout<<"failed load fv.bin\n";
    exit(1);
  }
  size_t size = SQUARE_SIZE * int(pos_n);
#if 0
  if(fread(pc_on_sq2,sizeof(Kvalue),size,pf) != size){
    std::cout<<"failed load fv.bin2\n";
    exit(1);
  }
#endif
  size = SQUARE_SIZE * int(fe_end) * int(fe_end);
  if(fread(pc_on_sq,sizeof(Kvalue),size,pf) != size){
    std::cout<<"failed load fv.bin2\n";
    exit(1);
  }
  size = SQUARE_SIZE * SQUARE_SIZE * int(kkp_end);
  if(fread(akkp,sizeof(Kvalue),size,pf) != size){
    std::cout<<"failed load fv.bin3\n";
    exit(1);
  }
  fclose(pf);
  std::cout<<"end\n";
#if 0
  convert_fv();
  int a,b,c;
  a = bb_to_array(70);
  b = bb_to_array(43) + pc_on_sq_index[PAWN_B];
  c = bb_to_array(34) + pc_on_sq_index[ROOK_W];
  ASSERT(pc_on_sq[a][b][c] == PcPcOnSqAny(a,b,c));
  ASSERT(pc_on_sq[a][b][c] == 9866);
#endif
}

#if 0
void convert_fv(){
 SQUARE_FOREACH(sq){
   for(int i = 0; i < int(fe_end); i++){
     for(int j = 0; j < int(fe_end); j++){
       pc_on_sq[sq][i][j] = PcPcOnSqAny(sq,i,j);
     }
   }
 }
}
#endif
Value evaluate_material(const Position & pos){

  Value value = Value(0);

  value += piece_value(ROOK_B)   * pos.hand(BLACK).num(ROOK_H);
  value += piece_value(BISHOP_B) * pos.hand(BLACK).num(BISHOP_H);
  value += piece_value(GOLD_B)   * pos.hand(BLACK).num(GOLD_H);
  value += piece_value(SILVER_B) * pos.hand(BLACK).num(SILVER_H);
  value += piece_value(KNIGHT_B) * pos.hand(BLACK).num(KNIGHT_H);
  value += piece_value(LANCE_B)  * pos.hand(BLACK).num(LANCE_H);
  value += piece_value(PAWN_B)   * pos.hand(BLACK).num(PAWN_H);

  value -= piece_value(ROOK_B)   * pos.hand(WHITE).num(ROOK_H);
  value -= piece_value(BISHOP_B) * pos.hand(WHITE).num(BISHOP_H);
  value -= piece_value(GOLD_B)   * pos.hand(WHITE).num(GOLD_H);
  value -= piece_value(SILVER_B) * pos.hand(WHITE).num(SILVER_H);
  value -= piece_value(KNIGHT_B) * pos.hand(WHITE).num(KNIGHT_H);
  value -= piece_value(LANCE_B)  * pos.hand(WHITE).num(LANCE_H);
  value -= piece_value(PAWN_B)   * pos.hand(WHITE).num(PAWN_H);

  SQUARE_FOREACH(sq){
    const auto p = pos.square(sq);
    value += piece_value(p);
  }
  ASSERT(value > -VALUE_EVAL_MAX && value < VALUE_EVAL_MAX);
  return value;
}
template<Color c>Value evaluate(Position & pos){

  ASSERT(pos.is_ok());
  ASSERT(pos.turn() == c);
  ASSERT(pos.ply() >= 0);
//  ASSERT(!pos.in_checked());

  search_info.try_eval_++;
  Value value = VALUE_ZERO;
#ifndef LEARN
  if(!pos.eval_is_empty()){
    search_info.try_stored_eval_++;
    return pos.eval()/FV_SCALE;
  }
  if(probe(pos.key(),pos.hand_b(),value)){
    pos.set_eval(value);
    search_info.try_stored_eval2_++;
    value /= FV_SCALE;
    return value;
  }
#endif

  EvalIndex list0[52],list1[52];

#ifndef LEARN
  if(calc_diff<c>(pos,value,list0,list1)){
    pos.set_eval(value);
    store(pos.key(),pos.hand_b(),value);
    value /= FV_SCALE;
    return value;
  }
#endif
  u32 nlist = make_list<c,false>(pos,list0,list1,value);
  Value sum = VALUE_ZERO;
  const auto bking = bb_to_array(pos.king_sq(BLACK));
  const auto wking = opp_square(bb_to_array(pos.king_sq(WHITE)));

  for(u32 i = 0; i < nlist; ++i){
    const EvalIndex k0 = list0[i];
    const EvalIndex k1 = list1[i];
    for(u32 j = 0; j <= i; ++j){
      const EvalIndex l0 = list0[j];
      const EvalIndex l1 = list1[j];
      sum += pc_pc_on_sq(bking,k0,l0);
      sum -= pc_pc_on_sq(wking,k1,l1);
    }
  }
  value += sum;
  value += pos.material() * FV_SCALE;
  value = (c == BLACK) ? value : -(value);
  pos.set_eval(value);
  store(pos.key(),pos.hand_b(),value);

  value /= FV_SCALE;

  ASSERT(value > -VALUE_EVAL_MAX && value < VALUE_EVAL_MAX);

  return value;
}
Value evaluate(Position & pos){
  return pos.turn() ? evaluate<WHITE>(pos)
                    : evaluate<BLACK>(pos);
}
template<Color c, bool last_move_is_tail>u32 make_list
  (Position & pos, EvalIndex list0[],
    EvalIndex list1[], Value & value){

  ASSERT(pos.is_ok());
  ASSERT(pos.ply() > 0);

  u32 nlist = 14;
  const auto bking_org = pos.king_sq(BLACK);
  const auto wking_org = pos.king_sq(WHITE);
  const auto bking = bb_to_array(bking_org);
  const auto wking = bb_to_array(wking_org);
  const auto bking2 = opp_square(bking);
  const auto wking2 = opp_square(wking);
  const auto to = pos.cur_move(pos.ply() - 1).to();

  ASSERT(square_is_ok(to));

  Bitboard occ_b = pos.color_bb(BLACK);
  Bitboard occ_w = pos.color_bb(WHITE);
  occ_b.Xor(bking_org);//delete king
  occ_w.Xor(wking_org);//delete king
  if(last_move_is_tail){
    if(c == BLACK){//delete last move;
      ASSERT(occ_w.is_seted(to));
      ASSERT(!occ_b.is_seted(to));
      occ_w.Xor(to);
    }else{
      ASSERT(occ_b.is_seted(to));
      ASSERT(!occ_w.is_seted(to));
      occ_b.Xor(to);
    }
  }
  Square sq;
  Hand hand_b = pos.hand(BLACK);
  Hand hand_w = pos.hand(WHITE);

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

  if(!last_move_is_tail){
    value += kkp(bking, wking, kkp_hand_pawn   + hand_b.num(PAWN_H));
    value += kkp(bking, wking, kkp_hand_lance  + hand_b.num(LANCE_H));
    value += kkp(bking, wking, kkp_hand_knight + hand_b.num(KNIGHT_H));
    value += kkp(bking, wking, kkp_hand_silver + hand_b.num(SILVER_H));
    value += kkp(bking, wking, kkp_hand_gold   + hand_b.num(GOLD_H));
    value += kkp(bking, wking, kkp_hand_bishop + hand_b.num(BISHOP_H));
    value += kkp(bking, wking, kkp_hand_rook   + hand_b.num(ROOK_H));

    value -= kkp(wking2, bking2, kkp_hand_pawn   + hand_w.num(PAWN_H));
    value -= kkp(wking2, bking2, kkp_hand_lance  + hand_w.num(LANCE_H));
    value -= kkp(wking2, bking2, kkp_hand_knight + hand_w.num(KNIGHT_H));
    value -= kkp(wking2, bking2, kkp_hand_silver + hand_w.num(SILVER_H));
    value -= kkp(wking2, bking2, kkp_hand_gold   + hand_w.num(GOLD_H));
    value -= kkp(wking2, bking2, kkp_hand_bishop + hand_w.num(BISHOP_H));
    value -= kkp(wking2, bking2, kkp_hand_rook   + hand_w.num(ROOK_H));

  }
  BB_FOR_EACH(occ_b,sq,
      const auto p = pos.square(sq);
      sq = bb_to_array(sq);
      list0[nlist] = pc_on_sq_index[p] + EvalIndex(sq);
      list1[nlist] = pc_on_sq_index[opp_piece(p)] + EvalIndex(opp_square(sq));
      ++nlist;
      ASSERT(nlist < 52);
      if(!last_move_is_tail){
        value += kkp(bking,wking,kkp_index[p] + EvalIndex(sq));
      }
      );
  BB_FOR_EACH(occ_w,sq,
      const auto p = pos.square(sq);
      sq = bb_to_array(sq);
      list0[nlist] = pc_on_sq_index[p] + EvalIndex(sq);
      list1[nlist] = pc_on_sq_index[opp_piece(p)] + EvalIndex(opp_square(sq));
      ++nlist;
      ASSERT(nlist < 52);
      if(!last_move_is_tail){
        value -= kkp(wking2,bking2,kkp_index[opp_piece(p)] + EvalIndex(opp_square(sq)));
      }
      );
  if(last_move_is_tail){//add last move at tail
    const auto p = pos.square(to);
    const auto sq = bb_to_array(to);
    list0[nlist] = pc_on_sq_index[p] + EvalIndex(sq);
    list1[nlist] = pc_on_sq_index[opp_piece(p)] + EvalIndex(opp_square(sq));
    ++nlist;
  }
  return nlist;
}
template<Color c>bool calc_diff(Position & pos,Value & value, EvalIndex list0[], EvalIndex list1[]){

  ASSERT(pos.ply() > 0);
  ASSERT(pos.is_ok());

  if(pos.eval_is_empty(pos.ply()-1)){//前回の評価値が無いなら差分評価出来ない
    return false;
  }
  const auto to = pos.cur_move(pos.ply() - 1).to();
  if(!square_is_ok(to)){
    pos.print();
    std::cout<<to<<std::endl;
    std::cout<<pos.cur_move(pos.ply()-1).str()<<std::endl;
    pos.dump();
  }
  ASSERT(square_is_ok(to));
  auto pc = pos.square(to);
  auto pt = piece_to_piece_type(pc);
  if(pt == KING){//前回の手が玉を動かす手なら差分評価しない
    return false;
  }
  const auto from = pos.cur_move(pos.ply() - 1).from();
  u32 nlist = make_list<c,true>(pos,list0,list1,value);
  Value diff = VALUE_ZERO;
  //toの位置価値をたす
  diff += doapc<c>(pos,pc,to,list0,list1,nlist);
  //toのlistを消す
  --nlist;
  if(square_is_drop(from)){
    //handが減った分の位置価値を足す
    const auto hp = piece_type_to_hand_piece(pt);
    const auto hand = pos.hand(opp_color(c));
    auto num = hand.num(hp);
    // printf("hand after分\n");
    diff += doacapt<c>(pos,hp,num,list0,list1,nlist);
    //handを打つ前の分の位置価値を引く
    ++list0[pc_on_sq_hand_index[hp] +     opp_color(c)];
    ++list1[pc_on_sq_hand_index[hp] + 1 - opp_color(c)];
    ++num;
    diff -= doacapt<c>(pos,hp,num,list0,list1,nlist);

  }else{
    auto cap = pos.capture(pos.ply()-1);
    auto capt = piece_to_piece_type(cap);
    if(cap){
      if(c == BLACK){
        diff -= piece_type_value_ex(capt) * FV_SCALE;
      }else{
        diff += piece_type_value_ex(capt) * FV_SCALE;
      }
      //handが増えた分の位置価値を足す
      const auto hp = piece_to_hand_piece(cap);
      const auto hand = pos.hand(opp_color(c));
      auto num = hand.num(hp);
      diff += doacapt<c>(pos,hp,num,list0,list1,nlist);

      --list0[pc_on_sq_hand_index[hp] +     opp_color(c)];
      --list1[pc_on_sq_hand_index[hp] + 1 - opp_color(c)];
      --num;
      //handを打つ前の分の位置価値を引く
      diff -= doacapt<c>(pos,hp,num,list0,list1,nlist);

      //capの位置価値を引く
      const auto cvt_to = bb_to_array(to);
      list0[nlist]   = pc_on_sq_index[cap] + cvt_to;
      list1[nlist++] = pc_on_sq_index[opp_piece(cap)] + opp_square(cvt_to);
      diff -= doapc<opp_color(c)>(pos,cap,to,list0,list1,nlist);
    }
    if(pos.cur_move(pos.ply()-1).is_prom()){
      pc = unprom_piece(pc);
      pt = unprom_piece_type(pt);
      if(c == BLACK){
        diff -= piece_type_value_pm(pt) * FV_SCALE;
      }else{
        diff += piece_type_value_pm(pt) * FV_SCALE;
      }
    }
    const auto cvt_from = bb_to_array(from);
    list0[nlist]   = pc_on_sq_index[pc] + cvt_from;
    list1[nlist++] = pc_on_sq_index[opp_piece(pc)] + opp_square(cvt_from);
    diff -= doapc<c>(pos,pc,from,list0,list1,nlist);
  }
  value = diff + ((c == BLACK) ? -pos.eval(pos.ply() - 1) : pos.eval(pos.ply() - 1));
  if(c == WHITE){
    value = -value;
  }
  return true;
}
//盤上の駒の差分の計算
template<Color eval_col>Value doapc(Position & pos, const Piece pc, const Square xy, EvalIndex list0[], EvalIndex list1[], const u32 nlist){

  ASSERT(pos.is_ok());
  ASSERT(piece_is_ok(pc));
  ASSERT(square_is_ok(xy));
  ASSERT(nlist < 52);

  const auto sq = bb_to_array(xy);//TODO bb_to_array is very slow!
  const auto pt = piece_to_piece_type(pc);
  const auto list0_index = pc_on_sq_index[          pc]  + EvalIndex(           sq );
  const auto list1_index = pc_on_sq_index[opp_piece(pc)] + EvalIndex(opp_square(sq));
  const auto bking = bb_to_array(pos.king_sq(BLACK));
  const auto wking = bb_to_array(pos.king_sq(WHITE));
  const auto bking2 = opp_square(bking);
  const auto wking2 = opp_square(wking);
  Value diff = VALUE_ZERO;
  //kppの計算
  for(int i = 0; i < int(nlist); ++i){
    diff += pc_pc_on_sq(bking,EvalIndex(list0_index),list0[i]);
    diff -= pc_pc_on_sq(wking2,EvalIndex(list1_index),list1[i]);
  }
  //kkpの計算
  if(eval_col == BLACK){//逆になる
    diff -= kkp(wking2,  bking2,  kkp_index[pt] + EvalIndex(opp_square(sq)));
  }else{
    diff += kkp(bking, wking, kkp_index[pt] + EvalIndex(sq));
  }
  return diff;
}
//持ち駒時の差分の計算
template<Color eval_col>Value doacapt(Position & pos, const HandPiece pc, const int hand_num,
                               EvalIndex list0[], EvalIndex list1[], const u32 nlist){

  ASSERT(pos.is_ok());
  ASSERT(nlist < 52);

  Value diff = VALUE_ZERO;
  const int list0_index = pc_on_sq_hand_index[pc]     + opp_color(eval_col); //先手の時　0　後手の時1
  const int list0_value = list0[list0_index];
  const int list1_index = pc_on_sq_hand_index[pc] + 1 - opp_color(eval_col);
  const int list1_value = list1[list1_index];
  const auto bking = bb_to_array(pos.king_sq(BLACK));
  const auto wking = bb_to_array(pos.king_sq(WHITE));
  const auto bking2 = opp_square(bking);
  const auto wking2 = opp_square(wking);

  ASSERT(list0_index != list1_index);

  //kppの計算
  for(int i = 0; i < int(nlist); ++i){
    diff += pc_pc_on_sq(bking, EvalIndex(list0_value), list0[i]);
    diff -= pc_pc_on_sq(wking2,EvalIndex(list1_value), list1[i]);
  }
  //kkpの計算
  if(eval_col == BLACK){
    diff -= kkp(wking2,  bking2,  EvalIndex(kkp_hand_index[pc]) + EvalIndex(hand_num));
  }else{
    diff += kkp(bking,    wking,  EvalIndex(kkp_hand_index[pc]) + EvalIndex(hand_num));
  }
  return diff;
}
void print_material(){

  PIECE_TYPE_FOREACH(p){
    std::cout<<str_piece_type_name(p)<<boost::format(" : %d ") % p_value[p];
    if(p == KING){
      std::cout<<"\n";
    }
  }
  std::cout<<"\n";
}
Value evaluate_nyugyoku(Position & pos){

  const auto bking = opp_square(pos.king_sq(BLACK));
  const auto wking = pos.king_sq(WHITE);
  const auto bk_rank = square_to_rank(bking);
  const auto wk_rank = square_to_rank(wking);
  Value value = VALUE_ZERO;

  value += nyugyoku_bounus[bk_rank] - nyugyoku_bounus[wk_rank];

  //eval material in nyugyoku
  if(bk_rank + wk_rank >= 7){

    int black_point = 0;
    auto five_point = pos.color_bb(BLACK) & pos.piece_bb(ROOK,BISHOP,PROOK,PBISHOP);
    auto one_point = pos.color_bb(BLACK) & (~five_point);
    black_point = 5 * five_point.pop_cnt() + one_point.pop_cnt();
    const Hand hand = pos.hand(BLACK);
    black_point +=  hand.num(PAWN_H)   + hand.num(LANCE_H)
                  + hand.num(KNIGHT_H) + hand.num(SILVER_H)
                  + hand.num(GOLD_H)   + ( 5 * hand.num(BISHOP_H))
                  + ( 5 * hand.num(ROOK_H));
    int x = black_point - 28;
    //TODO clean up
    int filter_point = (x > 4 ? x+4*3 : x < -4 ? x-4*3 : 4 * x);
    int z = (int(bk_rank + wk_rank) - 6) * filter_point * 4;
    value += Value(z*4);
  }
  return value * FV_SCALE;
}
void clear_weight(){

  std::cout<<"weight cleaning...";

  p_value[PAWN]    = Value(100);
  p_value[KNIGHT]  = Value(300);
  p_value[LANCE]   = Value(300);
  p_value[SILVER]  = Value(400);
  p_value[GOLD]    = Value(500);
  p_value[BISHOP]  = Value(600);
  p_value[ROOK]    = Value(700);
  p_value[PPAWN]   = Value(400);
  p_value[PLANCE]  = Value(400);
  p_value[PKNIGHT] = Value(400);
  p_value[PSILVER] = Value(500);
  p_value[PBISHOP] = Value(800);
  p_value[PROOK]   = Value(1000);

  set_piece_value();

  for(int i = 0; i < int(SQUARE_SIZE); i++){
    for(int j = 0; j < int(SQUARE_SIZE); j++){
      for(int k = 0; k <int(kkp_end); k++){
        akkp[i][j][k] = 0;
      }
    }
  }
  for(int i = 0; i < int(SQUARE_SIZE); i++){
    for(int j = 0; j < int(fe_end); j++){
      for(int k = 0; k <int(fe_end); k++){
        pc_on_sq[i][j][k] = 0;
      }
    }
  }
  std::cout<<"end\n";
}
