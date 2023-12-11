#include "myboost.h"
#include "bitboard.h"
#include "evaluator.h"
#include "position.h"
#include "piece.h"
#include "move.h"
#include "check_info.h"
#include "random.h"
#include "generator.h"
#include <iostream>
#include <string>
#include <locale>
#include <limits.h>
#include <boost/lexical_cast.hpp>
#include <boost/thread/thread.hpp>

#define OUT g_tee

#ifdef TLP

boost::mutex g_out_lock;

#endif
// const Piece hirate_position[SQUARE_SIZE] = {
//   LANCE_W, KNIGHT_W , EMPTY   , GOLD_W , KING_W , GOLD_W , SILVER_W , KNIGHT_W , LANCE_W,
//   EMPTY  , ROOK_W   , EMPTY   , EMPTY  , EMPTY  , EMPTY  , EMPTY    , BISHOP_W , EMPTY  ,
//   PAWN_W , PAWN_W   , EMPTY   , PAWN_W , PAWN_W , PAWN_W , EMPTY    , PAWN_W   , PAWN_W ,
//   EMPTY  , EMPTY    , PAWN_B  , EMPTY  , EMPTY  , EMPTY  , EMPTY    , EMPTY    , EMPTY  ,
//   EMPTY  , EMPTY    , EMPTY   , EMPTY  , EMPTY  , EMPTY  , EMPTY    , EMPTY    , EMPTY  ,
//   EMPTY  , EMPTY    , EMPTY   , EMPTY  , EMPTY  , EMPTY  , PAWN_W   , EMPTY    , EMPTY  ,
//   PAWN_B , PAWN_B   , EMPTY   , PAWN_B , PAWN_B , PAWN_B , EMPTY    , PAWN_B   , PAWN_B ,
//   EMPTY  , BISHOP_B , EMPTY   , EMPTY  , EMPTY  , EMPTY  , EMPTY    , ROOK_B   , EMPTY  ,
//   LANCE_B, KNIGHT_B , SILVER_B, GOLD_B , KING_B , GOLD_B , EMPTY    , KNIGHT_B , LANCE_B,
// };
const Piece hirate_position[SQUARE_SIZE] = {
  LANCE_W, KNIGHT_W , SILVER_W, GOLD_W , KING_W , GOLD_W , SILVER_W , KNIGHT_W , LANCE_W,
  EMPTY  , ROOK_W   , EMPTY   , EMPTY  , EMPTY  , EMPTY  , EMPTY    , BISHOP_W , EMPTY  ,
  PAWN_W , PAWN_W   , PAWN_W  , PAWN_W , PAWN_W , PAWN_W , PAWN_W   , PAWN_W   , PAWN_W ,
  EMPTY  , EMPTY    , EMPTY   , EMPTY  , EMPTY  , EMPTY  , EMPTY    , EMPTY    , EMPTY  ,
  EMPTY  , EMPTY    , EMPTY   , EMPTY  , EMPTY  , EMPTY  , EMPTY    , EMPTY    , EMPTY  ,
  EMPTY  , EMPTY    , EMPTY   , EMPTY  , EMPTY  , EMPTY  , EMPTY    , EMPTY    , EMPTY  ,
  PAWN_B , PAWN_B   , PAWN_B  , PAWN_B , PAWN_B , PAWN_B , PAWN_B   , PAWN_B   , PAWN_B ,
  EMPTY  , BISHOP_B , EMPTY   , EMPTY  , EMPTY  , EMPTY  , EMPTY    , ROOK_B   , EMPTY  ,
  LANCE_B, KNIGHT_B , SILVER_B, GOLD_B , KING_B , GOLD_B , SILVER_B , KNIGHT_B , LANCE_B,
};

Key key_seed[COLOR_SIZE][PIECE_TYPE_SIZE][SQUARE_SIZE];
Key singular_key_seed;
void init_key(){

  for(int i = 0; i < int(COLOR_SIZE); i++){
    for(int j = 0; j < int(PIECE_TYPE_SIZE); j++){
      for(int k = 0; k < int(SQUARE_SIZE); k++){
        key_seed[i][j][k] = random64();
      }
    }
  }
  singular_key_seed = random64();
}
void Position::print()const{

#ifdef TLP
  boost::mutex::scoped_lock lk(g_out_lock);
#endif
  printf("key:%llu ply:%d rep_ply:%d\n",key_[ply_],ply_,rep_ply_);
  if(turn_ == BLACK){
    std::cout<<"BLACK\n";
  }else if(turn_ == WHITE){
    std::cout<<"WHITE\n";
  }else{
    std::cout<<"TURN ERROR\n";
  }
  std::cout<<"飛:"<<hand_[WHITE].num(ROOK_H)
           <<" 角:"<<hand_[WHITE].num(BISHOP_H)
           <<" 金:"<<hand_[WHITE].num(GOLD_H)
           <<" 銀:"<<hand_[WHITE].num(SILVER_H)
           <<" 桂:"<<hand_[WHITE].num(KNIGHT_H)
           <<" 香:"<<hand_[WHITE].num(LANCE_H)
           <<" 歩:"<<hand_[WHITE].num(PAWN_H)<<std::endl;
  for(int xy = 0; xy < 81; xy++){
    if(square_[array_to_bb(xy)] == EMPTY || piece_color(square_[array_to_bb(xy)]) == BLACK){
      std::cout<<" ";
    }else{
      std::cout<<"v";
    }
    std::cout<<str_piece_name(square_[array_to_bb(xy)]);
    if((xy+1)%9 == 0){
      std::cout<<"\n";
    }
  }
  std::cout<<"飛:"<<hand_[BLACK].num(ROOK_H)
           <<" 角:"<<hand_[BLACK].num(BISHOP_H)
           <<" 金:"<<hand_[BLACK].num(GOLD_H)
           <<" 銀:"<<hand_[BLACK].num(SILVER_H)
           <<" 桂:"<<hand_[BLACK].num(KNIGHT_H)
           <<" 香:"<<hand_[BLACK].num(LANCE_H)
           <<" 歩:"<<hand_[BLACK].num(PAWN_H)<<std::endl;
  std::cout<<out_sfen()<<std::endl;
}
bool Position::init_position(){

  COLOR_FOREACH(c){
    color_bb_[c].set(0ull,0ull);
  }
  PIECE_TYPE_FOREACH(i){
    piece_bb_[i].set(0ull,0ull);
  }
  SQUARE_FOREACH(i){
    square_[i] = PIECE_SIZE;
  }
  for(int i = 0; i < MAX_PLY; i++){
    capture_[i] = EMPTY;
    checker_bb_[i].set(0,0);
    material_[i] = Value(INT_MAX);
    eval_[i] = STORED_EVAL_IS_EMPTY;
    trans_move_[i] = move_none();
    cur_move_[i] = move_none();
    skip_move_[i] = move_none();
    reduction_[i] = DEPTH_NONE;
  }
  for(int i = 0; i < Position::MAX_REP; i++){
    hand_b_[i].set(0);
    key_[i] = 0ull;

  }
  hand_[BLACK].set(0ull);
  hand_[WHITE].set(0ull);
  turn_ = COLOR_ILLEGAL;
  rep_ply_ = ply_ = 0;
  node_num_ = 0ULL;
  skip_null_ = false;
  for(int i = 0; i < MAX_PLY; i++){
    psort_[i] = nullptr;
    pci_[i] = nullptr;
    killer_[i].init();
  }
  tlp_stop_ = false;
#ifdef TLP
  for(int i = 0; i < TLP_MAX_THREAD; i++){
    tlp_sibling_[i] = nullptr;
  }
  tlp_parent_ = nullptr;
  tlp_used_ = false;
  tlp_slot_ = 0;
  tlp_alpha_ = -VALUE_NONE;
  tlp_beta_ = VALUE_NONE;
  tlp_best_value_ = VALUE_NONE;
  tlp_sibling_num_ = 0;
  tlp_depth_ = DEPTH_NONE;
  tlp_id_ = 0;
  tlp_cut_node_ = false;
  tlp_node_type_ = PV_NODE;
#endif
  return true;
}
//TODO コピーコンストラクタにする
void Position::init_position(const Position & p){

  //TODO ok?
  color_bb_[BLACK] = p.color_bb(BLACK);
  color_bb_[WHITE] = p.color_bb(WHITE);
  PIECE_TYPE_FOREACH(i){
    piece_bb_[i] = p.piece_bb(i);
  }
  piece_bb_[OCCUPIED] = p.occ_bb();
  SQUARE_FOREACH(i){
    square_[i] = p.square(i);
  }

  for(int i = std::max(p.ply()-2,0); i <= p.ply(); i++){
    capture_[i]            = p.capture(i);
    checker_bb_[i]         = p.checker_bb(i);
    material_[i]           = p.material(i);
    eval_[i]               = p.eval(i);
    trans_move_[i]         = p.trans_move(i);
    cur_move_[i]           = p.cur_move(i);
    skip_move_[i]          = p.skip_move(i);
    reduction_[i]          = p.reduction(i);
  }
  for(int i = 0; i <= p.rep_ply_; i++){
    hand_b_[i] = p.hand_b(i);
    key_[i]    = p.key(i);
  }
  for(int i = p.ply(); i < MAX_PLY; i++){
    killer_[i] = p.killer_[i];
  }
  for(int i = 0; i <= p.ply()+1; ++i){
    for(int j = 0; j <= p.ply()+1;j++){
      pv_.set_move(p.pv(i,j),i,j);
    }
  }
  hand_[BLACK] = p.hand(BLACK);
  hand_[WHITE] = p.hand(WHITE);
  turn_ = p.turn();
  rep_ply_ = p.rep_ply_;
  ply_ = p.ply();
  node_num_ = 0;
  skip_null_ = p.skip_null_;
#ifdef TLP
  psort_[ply()] = p.psort_[ply()];
#endif
}
bool Position::init_position
(const Piece array[], const Color turn, const Hand & hand_b, const Hand & hand_w){

  this->init_position();
  turn_ = turn;
  hand_[BLACK] = hand_b;
  hand_[WHITE] = hand_w;
  int piece_num[PIECE_TYPE_SIZE] = {};
  piece_num[ROOK]+=hand_[BLACK].num(ROOK_H)+hand_[WHITE].num(ROOK_H);
  piece_num[BISHOP]+=hand_[BLACK].num(BISHOP_H)+hand_[WHITE].num(BISHOP_H);
  piece_num[GOLD]+=hand_[BLACK].num(GOLD_H)+hand_[WHITE].num(GOLD_H);
  piece_num[SILVER]+=hand_[BLACK].num(SILVER_H)+hand_[WHITE].num(SILVER_H);
  piece_num[KNIGHT]+=hand_[BLACK].num(KNIGHT_H)+hand_[WHITE].num(KNIGHT_H);
  piece_num[LANCE]+=hand_[BLACK].num(LANCE_H)+hand_[WHITE].num(LANCE_H);
  piece_num[PAWN]+=hand_[BLACK].num(PAWN_H)+hand_[WHITE].num(PAWN_H);

  SQUARE_FOREACH(xy){
    const auto xy2 = array_to_bb(xy);
    square_[xy2] = array[xy];
    if(square_[xy2]){
      if(!piece_is_ok(array[xy])){
        goto error;
      }
      int tmp = square_[xy2];
      if(tmp > ROOK_B){
        tmp-=(int)WHITE_FLAG;
      }
      piece_bb_[tmp].Or(xy2);
      color_bb_[piece_color(square_[xy2])].Or(xy2);
      PieceType p = piece_to_piece_type(square_[xy2]);
      if(piece_type_is_promed(p)){
        p = unprom_piece_type(p);
      }
      piece_num[p]++;
    }
  }
  piece_bb_[OCCUPIED] = color_bb_[BLACK] | color_bb_[WHITE];
#ifndef PEFECT_RECORD
  if(piece_bb_[KING].pop_cnt() != 2){
    std::cout<<"king num few fail "<<piece_bb_[KING].pop_cnt()<<std::endl;
    goto error;
  }
#endif
  set_checker_bb(make_checker_bb_debug(),0);
  material_[ply_] = evaluate_material(*this);
  key_[ply_] = make_key();
  hand_b_[rep_ply_] = hand_[BLACK];
#ifndef PEFECT_RECORD
  if(!is_ok()){
    std::cout<<"Position.is_ok() fail\n";
    goto error;
  }
  if(piece_num[PAWN] >=19){
    std::cout<<"too many p\n";
    goto error;
  }
  if(piece_num[LANCE] >=5){
    std::cout<<"too many l\n";
    goto error;
  }
  if(piece_num[KNIGHT] >=5){
    std::cout<<"too many n\n";
    goto error;
  }
  if(piece_num[SILVER] >=5){
    std::cout<<"too many s\n";
    goto error;
  }
  if(piece_num[GOLD] >=5){
    std::cout<<"too many g\n";
    goto error;
  }
  if(piece_num[BISHOP] >=3){
    std::cout<<"too many b\n";
    goto error;
  }
  if(piece_num[ROOK] >=3){
    std::cout<<"too many r\n";
    goto error;
  }
#endif
  return true;
error:
  std::cout<<"init position error\n";
  init_position();
  return false;
}
bool Position::init_position(const std::string s){

  int len = s.size();
  int sp = 0;
  Piece array[SQUARE_SIZE] = {
    EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
    EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
    EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
    EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
    EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
    EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
    EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
    EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
    EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  };
  Hand bh(0),wh(0);
  Color c = s.find(" w ") != std::string::npos ? WHITE : BLACK;
  Piece piece = EMPTY;
  std::locale loc;
  int i;
  for(i = 0; i < len; i++){
    if(sp >= SQUARE_SIZE){
      break;
    }
    std::string str = s.substr(i,1);
    if(false){
    }else if(str == "/" || str == "+"){
    }else if(std::isdigit(str[0],loc)){
      sp += boost::lexical_cast<int>(str);
    }else if((piece = sfen_to_piece(str)) != EMPTY){
      if(i > 0 && s.substr(i-1,1) == "+"){
        piece = prom_piece(piece);
      }
      array[sp++] = piece;
    }
  }
  int h[COLOR_SIZE][HAND_PIECE_SIZE] = {};
  if(s.find(" - ") != std::string::npos){
    bh.set(0);
    wh.set(0);
  }else{
    int num = 0;
    //hand b skip
    for(i = i+3; i < len; i++){
      std::string str = s.substr(i,1);
      if(str == " "){
        break;
      }
      else if(std::isdigit(str[0],loc)){
        if(num != 0){
          num = boost::lexical_cast<int>(str) + num*10;
        }else{
           num = boost::lexical_cast<int>(str);
        }
      }else if((piece = sfen_to_piece(str)) != EMPTY){
        Color tc = piece_color(piece);
        HandPiece hp = piece_to_hand_piece(piece);
        if(num == 0){ num = 1; }
        h[tc][hp] = num;
        num = 0;
      }
    }
    bh.set(ROOK_H,h[BLACK][ROOK_H],BISHOP_H,h[BLACK][BISHOP_H],GOLD_H,h[BLACK][GOLD_H],
        SILVER_H,h[BLACK][SILVER_H],KNIGHT_H,h[BLACK][KNIGHT_H],LANCE_H,h[BLACK][LANCE_H],
        PAWN_H,h[BLACK][PAWN_H]);
    wh.set(ROOK_H,h[WHITE][ROOK_H],BISHOP_H,h[WHITE][BISHOP_H],GOLD_H,h[WHITE][GOLD_H],
        SILVER_H,h[WHITE][SILVER_H],KNIGHT_H,h[WHITE][KNIGHT_H],LANCE_H,h[WHITE][LANCE_H],
        PAWN_H,h[WHITE][PAWN_H]);
  }
  return init_position(array,c,bh,wh);
}
void Position::init_hirate(){

  Hand b(0),w(0);
  this->init_position(hirate_position,BLACK,b,w);
  ASSERT(this->is_ok());
}
void Position::dump()const{

#ifdef LEARN
  return;
#endif
  std::cout<<"ply : "<<ply_<<std::endl;
  std::cout<<"node : "<<node_num_<<std::endl;
  std::cout<<"color"<<turn_<<std::endl;
  std::cout<<"hand b: "<<hand_[BLACK].value()
               <<" w: "<<hand_[WHITE].value()<<std::endl;
  this->print();
  for(int xy = 0; xy < 81; xy++){
    if(square_[xy]<10){
        std::cout<<" ";
    }
    std::cout<<square_[xy]<<",";
    if((xy+1)%9==0){
      std::cout<<"\n";
    }
  }
  std::cout<<"color bb\n";
  for(int i = 0; i<COLOR_SIZE; i++){
    std::cout<<i<<std::endl;
    std::cout<<"0:"<<color_bb_[i].bb(0)<<"\n";
    std::cout<<"1:"<<color_bb_[i].bb(1)<<"\n";
    color_bb_[i].print();
  }
  std::cout<<"piece bb\n";
  for(int i = 0; i<PIECE_TYPE_SIZE; i++){
    std::cout<<i<<piece_name[i]<<std::endl;
    std::cout<<"0:"<<piece_bb_[i].bb(0)<<"\n";
    std::cout<<"1:"<<piece_bb_[i].bb(1)<<"\n";
    piece_bb_[i].print();
  }
  printf("checker bb\n");
  checker_bb_[ply_].print();
  printf("cur_move\n");
  for(int i = 0; i <= ply(); i++){
    std::cout<<i<<" "<<cur_move(i).str()<<" check?:"<<checker_bb_[i].is_not_zero()<<" cap:"<<capture_[i]<<" eval:"<<eval_[i]<<std::endl;
  }
}
bool Position::is_ok()const{

  if(turn_!=BLACK && turn_!=WHITE){
    std::cout<<"COLOR is illegal"<<turn_<<std::endl;
    return false;
  }
// square -> bb
  for(Square xy = A1 ; xy < SQUARE_SIZE; xy++){
    if(square_[xy] == EMPTY){
      //color bb
        if(color_bb_[BLACK].is_seted(xy)){
          std::cout<<"color_bb_BLACK error is seted"<<xy<<std::endl;
          this->dump();
          return false;
        }
        if(color_bb_[WHITE].is_seted(xy)){
          std::cout<<"color_bb_WHITE error is seted"<<xy<<std::endl;
          this->dump();
          return false;
        }
      //piece bb
      for(PieceType i = EMPTY_T; i < PIECE_TYPE_SIZE; i++){
        if(piece_bb_[i].is_seted(xy)){
          std::cout<<"piece_bb error is seted"<<i<<" : "<<xy<<std::endl;
          this->dump();
          return false;
        }
      }
      //OCCUPIED
      if(piece_bb_[OCCUPIED].is_seted(xy)){
          std::cout<<"OCC error is seted"<<xy<<std::endl;
          this->dump();
          return false;
      }
    }else{
      //color bb
      if(!color_bb_[piece_color(square_[xy])].is_seted(xy)){
        std::cout<<"color_bb error is empty "
                 <<piece_color(square_[xy])<<" "
                 <<xy<<" "
                 <<square_[xy]
                 <<std::endl;
        this->dump();
        return false;
      }
      //piece bb
      if(!piece_bb_[piece_to_piece_type(square_[xy])].is_seted(xy)){
        std::cout<<"piece_bb error is empty"
          <<square_[xy]
          <<" : "
          <<piece_to_piece_type(square_[xy])
          <<" : "
          <<xy
          <<std::endl;
        this->dump();
        return false;
      }
      //OCCUPIED
      if(!piece_bb_[OCCUPIED].is_seted(xy)){
          std::cout<<"OCC error is empty"<<xy<<std::endl;
          this->dump();
          return false;
      }
    }
  }
  //color_bb -> square
  for(Color c = BLACK; c < COLOR_SIZE; c++){
    Bitboard bb = color_bb_[c];
    while(bb.is_not_zero()){
      Square xy = bb.lsb<D>();
      if(square_[xy] == EMPTY){
        std::cout<<"color_bb stand but square is empty\n";
        this->dump();
        return false;
      }
      if(c != piece_color(square_[xy])){
        std::cout<<"color_bb stand but square is opp piece\n";
        this->dump();
        return false;
      }
    }
  }
  //piece_bb -> square
  for(PieceType p = PPAWN; p <= ROOK; p++){
    Bitboard bb = piece_bb_[p];
    while(bb.is_not_zero()){
      Square xy = bb.lsb<D>();
      if(square_[xy] == EMPTY){
        std::cout<<"piece_bb stand but square is empty "<<xy<<std::endl;
        this->dump();
        return false;
      }

      if(p != piece_to_piece_type(square_[xy])){
        std::cout<<"piece_bb stand but square is not same piece "<<xy<<std::endl;
        this->dump();
        return false;
      }
    }
  }

  if(occ_bb()!=(color_bb_[BLACK] | color_bb_[WHITE])){
    std::cout<<"occ != color_bb\n";
    this->dump();
    return false;
  }

  Bitboard bb;
  bb = piece_bb(PAWN_B) | piece_bb(LANCE_B) | piece_bb(KNIGHT_B);
  while(bb.is_not_zero()){
    Square from = bb.lsb<D>();
    Rank r = square_to_rank(from);
    if(r == RANK_1){
      std::cout<<"can not move piece "<<from<<std::endl;
      this->dump();
      return false;
    }
  }
  bb = piece_bb(PAWN_W) | piece_bb(LANCE_W) | piece_bb(KNIGHT_W);
  while(bb.is_not_zero()){
    Square from = bb.lsb<D>();
    Rank r = square_to_rank(from);
    if(r == RANK_9){
      std::cout<<"can not move piece "<<from<<std::endl;
      this->dump();
      return false;
    }
  }
  bb = piece_bb(KNIGHT_B);
  while(bb.is_not_zero()){
    Square from = bb.lsb<D>();
    Rank r = square_to_rank(from);
    if(r == RANK_2){
      std::cout<<"can not move piece "<<from<<std::endl;
      this->dump();
      return false;
    }
  }
  bb = piece_bb(KNIGHT_W);
  while(bb.is_not_zero()){
    Square from = bb.lsb<D>();
    Rank r = square_to_rank(from);
    if(r == RANK_8){
      std::cout<<"can not move piece "<<from<<std::endl;
      this->dump();
      return false;
    }
  }

  #define CHECK_DOUBLE_PAWN(PPP,xx) do{\
    bb = piece_bb(PPP) & file_mask[FILE_A];\
    if(bb.pop_cnt()>=2){\
      std::cout<<"doble pawn "<<std::endl;\
      this->dump();\
      return false;\
    }\
  }while(0)

  CHECK_DOUBLE_PAWN(PAWN_B,FILE_A);
  CHECK_DOUBLE_PAWN(PAWN_B,FILE_B);
  CHECK_DOUBLE_PAWN(PAWN_B,FILE_C);
  CHECK_DOUBLE_PAWN(PAWN_B,FILE_D);
  CHECK_DOUBLE_PAWN(PAWN_B,FILE_E);
  CHECK_DOUBLE_PAWN(PAWN_B,FILE_F);
  CHECK_DOUBLE_PAWN(PAWN_B,FILE_G);
  CHECK_DOUBLE_PAWN(PAWN_B,FILE_H);
  CHECK_DOUBLE_PAWN(PAWN_B,FILE_I);

  CHECK_DOUBLE_PAWN(PAWN_W,FILE_A);
  CHECK_DOUBLE_PAWN(PAWN_W,FILE_B);
  CHECK_DOUBLE_PAWN(PAWN_W,FILE_C);
  CHECK_DOUBLE_PAWN(PAWN_W,FILE_D);
  CHECK_DOUBLE_PAWN(PAWN_W,FILE_E);
  CHECK_DOUBLE_PAWN(PAWN_W,FILE_F);
  CHECK_DOUBLE_PAWN(PAWN_W,FILE_G);
  CHECK_DOUBLE_PAWN(PAWN_W,FILE_H);
  CHECK_DOUBLE_PAWN(PAWN_W,FILE_I);

  #undef CHECK_DOUBLE_PAWN
  //check piece type num
  int tmp_piece_num[PIECE_TYPE_SIZE] = {0};

  tmp_piece_num[ROOK]+=hand_[BLACK].num(ROOK_H)+hand_[WHITE].num(ROOK_H);
  tmp_piece_num[BISHOP]+=hand_[BLACK].num(BISHOP_H)+hand_[WHITE].num(BISHOP_H);
  tmp_piece_num[GOLD]+=hand_[BLACK].num(GOLD_H)+hand_[WHITE].num(GOLD_H);
  tmp_piece_num[SILVER]+=hand_[BLACK].num(SILVER_H)+hand_[WHITE].num(SILVER_H);
  tmp_piece_num[KNIGHT]+=hand_[BLACK].num(KNIGHT_H)+hand_[WHITE].num(KNIGHT_H);
  tmp_piece_num[LANCE]+=hand_[BLACK].num(LANCE_H)+hand_[WHITE].num(LANCE_H);
  tmp_piece_num[PAWN]+=hand_[BLACK].num(PAWN_H)+hand_[WHITE].num(PAWN_H);
  SQUARE_FOREACH(sq){
    if(square_[sq]){
      PieceType p = piece_to_piece_type(square_[sq]);
      if(piece_type_is_promed(p)){
        p = unprom_piece_type(p);
      }
      tmp_piece_num[p]++;
    }
  }
  Bitboard tmp_bb = make_checker_bb_debug();
  if(checker_bb() != tmp_bb){
    printf("checker bb is not same\n");
    printf("answer is this\n");
    tmp_bb.print();
    checker_bb().print();
    this->dump();
    return false;
  }
  if(piece_bb_[KING].pop_cnt() != 2){
    printf("king is few %d\n",piece_bb_[KING].pop_cnt());
    this->dump();
    return false;
  }
  if(!ply_is_ok(ply())){
    printf("ply is not good %d\n",ply());
    this->dump();
    return false;
  }
  Value tmp_material = evaluate_material(*this);
  if(tmp_material != material_[ply_]){
    std::cout<<"material is not same\n";
    std::cout<<tmp_material<<std::endl;
    std::cout<<material_[ply_]<<std::endl;
    dump();
    return false;
  }
  u64 tmp_key = make_key();
  if(tmp_key != key_[rep_ply_]){
    std::cout<<"key is not same\n";
    std::cout<<tmp_key<<std::endl;
    std::cout<<key_[rep_ply_]<<std::endl;
    dump();
    return false;
  }
  if(hand_b_[rep_ply_] != hand_[BLACK]){
    std::cout<<"hand is not same\n";
    std::cout<<rep_ply_<<std::endl;
    std::cout<<hand_b_[rep_ply_].value()<<std::endl;
    std::cout<<hand_[BLACK].value()<<std::endl;
    dump();
    return false;
  }

  return true;
}
bool Position::is_ok(const Move move)const{

  if(!is_ok()){
    if(move.is_ok()){
      std::cout<<move.str()<<std::endl;
    }else{
      std::cout<<"move is illegal\n";
    }
    return false;
  }
  return true;
}
void Position::do_move(const Move & move, const CheckInfo & ci, const bool check){

  ASSERT(is_ok(move));
  ASSERT(move.is_ok(*this));
  //check move info
  const Square from = move.from();
  const Square to = move.to();
  const Color  me = turn();
  const Color  opp = opp_color(me);
  PieceType piece_t;
  Piece      piece;
  cur_move_[ply()] = move;
  material_[ply()+1] = material_[ply()];
  eval_[ply()+1] = STORED_EVAL_IS_EMPTY;
  key_[rep_ply_+1] = key_[rep_ply_];
  if(square_is_drop(from)){
    const HandPiece hand_p  = move.hand_piece();
    piece_t = hand_piece_to_piece_type(hand_p);
    piece = piece_type_to_piece(piece_t,me);
    dec_hand(hand_p,me);
    //put square(to)
    set_xor_bb(me,to);
    set_xor_bb(piece_t,to);
    set_xor_bb(OCCUPIED,to);
    set_square(piece,to);
    set_capture(EMPTY);
    key_[rep_ply_+1] ^= key_seed[me][piece_t][to];
    if(check){
      set_checker_bb(mask[to]);
    }else{
      set_checker_bb(ALL_ZERO_BB);
    }
  }
  else{
    piece = square(from);
    piece_t = piece_to_piece_type(piece);
    //remove square(from)
    set_xor_bb(me,from);
    set_xor_bb(piece_t,from);
    set_xor_bb(OCCUPIED,from);
    set_square(EMPTY,from);
    key_[rep_ply_+1] ^= key_seed[me][piece_t][from];

    const Piece capture = square(to);
    if(capture){
      const PieceType capture_t = piece_to_piece_type(capture);
      const PieceType unprom_cap_t = unprom_piece_type(capture_t);
      const HandPiece cap_hp = piece_type_to_hand_piece(unprom_cap_t);
      ASSERT(capture_t != KING);
      set_xor_bb(opp,to);
      set_xor_bb(capture_t,to);
      set_capture(capture);
      inc_hand(cap_hp,me);
      material_[ply()+1] += piece_value_ex(opp_piece(capture));
      key_[rep_ply_+1] ^= key_seed[opp][capture_t][to];
    }else{
      set_xor_bb(OCCUPIED,to);
      set_capture(EMPTY);
    }
    if(move.is_prom()){
      piece_t = prom_piece_type(piece_t);
      piece   = prom_piece(piece);
      material_[ply()+1] += piece_value_pm(piece);

    }else{
    }
    //put square(to)
    set_xor_bb(me,to);
    set_xor_bb(piece_t,to);
    set_square(piece,to);
    key_[rep_ply_+1] ^= key_seed[me][piece_t][to];

    Bitboard checker_bb(0,0);
    if(check){
      if(ci.check_sq(piece_t).is_seted(to)){
        checker_bb.Or(to);
      }
      if(ci.discover_checker().is_seted(from)){
        const auto ksq = king_sq(opp);
        switch(get_square_relation(from,ksq)){
          case RANK_RELATION:
            checker_bb |= attack_from<ROOK>(opp,ksq) & piece_bb(me,ROOK,PROOK);
          break;
          case FILE_RELATION:
          {
            Bitboard att = attack_from<ROOK>(me,ksq);
            checker_bb |= att & piece_bb(me,ROOK,PROOK);
            att &= get_pseudo_attack<LANCE>(opp,ksq);
            checker_bb |= att & piece_bb(me,LANCE);
          }
          break;
          case LEFT_UP_RELATION :
          case RIGHT_UP_RELATION :
            checker_bb |= attack_from<BISHOP>(opp,ksq) & piece_bb(me,BISHOP,PBISHOP);
          break;
          default:
          ASSERT(false);
          break;
        }
      }
    }else{
      ASSERT(!checker_bb.is_not_zero());
    }
    set_checker_bb(checker_bb);
  }
  key_[rep_ply_+1] = ~key_[rep_ply_+1];
  hand_b_[rep_ply_+1] = hand_[BLACK];
  inc_ply();
  ++rep_ply_;
  set_turn(opp);
  ASSERT(is_ok(move));
}
void Position::undo_move(const Move & move){

  ASSERT(is_ok());
  //check move info
  dec_ply();
  --rep_ply_;
  const Square from = move.from();
  const Square to = move.to();
  const Color  opp = turn();
  const Color  me = opp_color(opp);
  PieceType piece_t;
  Piece      piece;

  set_turn(me);
  if(square_is_drop(from)){
    const HandPiece hand_p  = move.hand_piece();
    piece_t = hand_piece_to_piece_type(hand_p);
    inc_hand(hand_p,me);
    //remove square(to)
    set_xor_bb(me,to);
    set_xor_bb(piece_t,to);
    set_xor_bb(OCCUPIED,to);
    set_square(EMPTY,to);
  }
  else{
    //remove square(to)
    set_xor_bb(me,to);
    piece = square(to);
    piece_t = piece_to_piece_type(piece);
    if(move.is_prom()){
      //remove prom piece
      set_xor_bb(piece_t,to);
      piece_t = unprom_piece_type(piece_t);
      piece   = unprom_piece(piece);

    }else{
      set_xor_bb(piece_t,to);
    }
    const Piece capture = this->capture();
    if(capture){
      const PieceType capture_t = piece_to_piece_type(capture);
      const PieceType unprom_cap_t = unprom_piece_type(capture_t);
      const HandPiece capture_hp = piece_type_to_hand_piece(unprom_cap_t);
      //ASSERT(color_bb_[opp].is_seted(to));
      set_xor_bb(opp,to);
      set_xor_bb(capture_t,to);
      set_square(capture,to);
      dec_hand(capture_hp,me);
      // don't need occupied
    }else{
      set_square(EMPTY,to);
      set_xor_bb(OCCUPIED,to);//need occupied
    }
    //put square(from)
    set_xor_bb(me,from);
    set_xor_bb(OCCUPIED,from);
    set_xor_bb(piece_t,from);
    set_square(piece,from);
  }
  ASSERT(is_ok());
  ASSERT(is_ok(move));
}
void Position::do_null_move(){

  ASSERT(is_ok());
  ASSERT(!in_checked());
  ASSERT(eval_[ply()] != STORED_EVAL_IS_EMPTY);
  const Color  me = turn();
  const Color  opp = opp_color(me);
  cur_move_[ply()] = move_pass();
  hand_b_[rep_ply_+1] = hand_[BLACK];
  material_[ply()+1] = material_[ply()];
  eval_[ply()+1] = -eval_[ply()];
  key_[rep_ply_+1] = ~key_[rep_ply_];
  set_checker_bb(ALL_ZERO_BB);
  set_capture(EMPTY);
  inc_ply();
  ++rep_ply_;
  set_turn(opp);
  ASSERT(is_ok());
}
void Position::undo_null_move(){

  ASSERT(is_ok());
  //check move info
  dec_ply();
  --rep_ply_;
  const Color  opp = turn();
  const Color  me = opp_color(opp);
  set_turn(me);
  ASSERT(is_ok());
}
Bitboard Position::hidden_checker(const Square king_sq,
                                  const Color checker_color,
                                  const Color pinned_color)const{

  ASSERT(square_is_ok(king_sq));
  ASSERT(color_is_ok(checker_color));
  ASSERT(color_is_ok(pinned_color));
  //ASSERT(piece_color(square(king_sq)) != checker_color);
  Bitboard checker = (( piece_bb(ROOK,PROOK)
                     & get_pseudo_attack<ROOK>(BLACK,king_sq))
                      |(piece_bb(BISHOP,PBISHOP)
                     & get_pseudo_attack<BISHOP>(BLACK,king_sq))
                      |(piece_bb(LANCE)
                     & get_pseudo_attack<LANCE>(opp_color(checker_color),king_sq)))
                    & color_bb(checker_color);
  Bitboard ret(0,0);
  while(checker.is_not_zero()){
    const auto sq = checker.lsb<D>();
    Bitboard btwn = get_between(sq,king_sq) & occ_bb();
    if(btwn.pop_cnt() == 1){//todo
      ret |= btwn & color_bb(pinned_color);
    }
  }
  return ret;
}

bool Position::move_is_check(const Move & move, const CheckInfo & ci)const{

  ASSERT(is_ok(move));
  ASSERT(move.is_ok(*this));
  ASSERT(ci.is_ok());
  const auto from = move.from();
  const auto to = move.to();
  if(square_is_drop(from)){
    //direct check
    const auto p = hand_piece_to_piece_type(move.hand_piece());
    return ci.check_sq(p).is_seted(to);
  }else{
    auto p = piece_to_piece_type(square(from));
    if(move.is_prom()){
      p = prom_piece_type(p);
    }
    if(ci.check_sq(p).is_seted(to)){//direct check
      return true;
    }
    //discover check
    return move_is_discover(from,to,ci.discover_checker());
    // const auto opp = opp_color(turn());
    // const auto ksq = king_sq(opp);
    // const auto fk_sr = get_square_relation(from,ksq);
    // //todo
    // return (fk_sr
    //         && fk_sr != get_square_relation(from,to)
    //         && ci.discover_checker().is_seted(from));
  }
}
bool Position::pseudo_is_legal(const Move & move,
                               const Bitboard & pinned)const{
  ASSERT(is_ok(move));
  ASSERT(move.is_ok(*this));
  const auto from = move.from();
  const auto to = move.to();
  return pseudo_is_legal(from,to,pinned);
}
bool Position::pseudo_is_legal(const Square from,
                               const Square to,
                               const Bitboard & pinned)const{
  return pseudo_is_legal(turn(),from,to,pinned);
}
bool Position::pseudo_is_legal(const Color c,
                               const Square from,
                               const Square to,
                               const Bitboard & pinned)const{

  if(square_is_drop(from)){
    //no check
    return true;
  }else{
    const auto piece = square(from);
    const auto piece_t = piece_to_piece_type(piece);
    if(piece_t == KING){
      const auto me = c;
      const auto opp = opp_color(me);
      return !attack_to(opp,to).is_not_zero();
    }else{
      if(pinned.is_seted(from)){
        const auto ksq = king_sq(c);
        return (get_square_relation(from,to)
                == get_square_relation(from,ksq));
      }
      return true;
    }
  }
}
template<Color c>bool Position::move_is_pseudo(const Move & move)const{

  ASSERT(c == turn());

  const auto from = move.from();
  const auto to = move.to();
  constexpr auto me = c;
  if(square_is_drop(from)){
    const auto hp = move.hand_piece();
    if(square(to)){
      return false;
    }
    if(!hand_[me].is_have(hp)){
      return false;
    }
    if(hp == PAWN_H){
      const auto occ = piece_bb(c,PAWN);
      const auto file = square_to_file(to);
      if((occ & file_mask[file]).is_not_zero()){
        return false;
      }
      const auto rank = square_to_rank(to);
      if(me == BLACK && rank == RANK_1){
        return false;
      }else if(me == WHITE && rank == RANK_9){
        return false;
      }else if(move_is_mate_with_pawn_drop(to)){
        return false;
      }
    }else if(hp == LANCE_H){
      const auto rank = square_to_rank(to);
      if(me == BLACK && rank == RANK_1){
        return false;
      }else if(me == WHITE && rank == RANK_9){
        return false;
      }
    }else if(hp == KNIGHT_H){
      const auto rank = square_to_rank(to);
      if(me == BLACK && rank <= RANK_2){
        return false;
      }else if(me == WHITE && rank >= RANK_8){
        return false;
      }
    }
  }else{
    const auto piece = square(from);
    const auto cap  = square(to);
    const auto prom = move.is_prom();
    if(!piece || piece_color(piece) != me){
      return false;
    }
    if(cap && piece_color(cap) == me){
      return false;
    }
    const auto piece_t = piece_to_piece_type(piece);
    const auto bb = get_pseudo_attack(piece_t,me,from);
    if(!bb.is_seted(to)){
      return false;
    }
    const auto bt = get_between(from,to);
    if((bt & occ_bb()).is_not_zero()){
      return false;
    }
    if(prom){
      if(!can_prom_piece_type(piece_t)){
        return false;
      }
      const auto fr = square_to_rank(from);
      const auto tr = square_to_rank(to);
      if(!is_prom_rank<me>(tr) && !is_prom_rank<me>(fr)){
        return false;
      }
    }else{
      const auto fr = square_to_rank(from);
      const auto tr = square_to_rank(to);
      if(is_prom_rank<me>(tr) || is_prom_rank<me>(fr)){
        if(piece_t == PAWN || piece_t == ROOK || piece_t == BISHOP){
          return false;
        }
      }
      if(me == BLACK && (piece_t == LANCE || piece_t == KNIGHT) &&tr <= RANK_2){
        return false;
      }
      else if(me == WHITE && (piece_t == LANCE || piece_t == KNIGHT) &&tr >= RANK_8){
        return false;
      }
    }
  }
  if(!move.is_ok_bonanza(*this)){
    return false;
  }
  return true;
}
Bitboard Position::make_checker_bb_debug()const{

  ASSERT(piece_bb_[KING].pop_cnt() == 2);
  const auto ksq = king_sq(turn());
  return attack_to(opp_color(turn()),ksq);
}
std::string Position::out_sfen()const{

  std::stringstream ret;
  int num = 0;
  SQUARE_FOREACH(xy){
    Square xy2 = array_to_bb(xy);
    //printf("xy : %d\n",xy2);
    Piece p = square(xy2);
    if(p == EMPTY){
      num++;
    }
    else{
      if(num!=0){
        ret<<num;
        num = 0;
      }
      if(promed_piece(p)){
        ret<<"+";
        p = unprom_piece(p);
      }
      ret<<piece_to_sfen(p);
    }
    if((xy+1)%9 ==0){
      if(num!=0){
        ret<<num;
        num = 0;
      }
      if(xy+1 != 81){
        ret<<"/";
      }
    }
  }
  ret<<" ";
  if(turn() == BLACK){
    ret<<"b ";
  }else{
    ret<<"w ";
  }
  if(hand_[BLACK].value() == 0 && hand_[WHITE].value() == 0){
    ret<<"-";
  }else{
    COLOR_FOREACH(tc){
      HAND_FOREACH(hp){
        int num = hand_[tc].num(hp);
        if(num != 0){
          if(num == 1){
          }else{
            ret<<num;
          }
          Piece p = hand_piece_to_piece(hp,tc);
          ret<<piece_to_sfen(p);
        }
      }
    }
  }
  ret<<" 1";
  return ret.str();
}
u64 Position::make_key()const{
  u64 ret = 0ULL;
  SQUARE_FOREACH(sq){
    if(square_[sq]){
      if(!piece_is_ok(square_[sq])){
        std::cout<<sq<<" "<<square_[sq]<<std::endl;
        ASSERT(false);
      }
      const auto col = piece_color(square_[sq]);
      const auto pt = piece_to_piece_type(square_[sq]);
      ret ^= key_seed[col][pt][sq];
    }
  }
  if(turn_ == WHITE){
    ret = ~ret;
  }
  return ret;
}
HandState Position::check_rep(const int limit_ply,const int limit_num, int & hit_point)const{

  ASSERT(is_ok());
  ASSERT(limit_ply >= 0);
  ASSERT(limit_ply < rep_ply_);
  ASSERT(limit_num > 0);

  const auto now = key_[rep_ply_];
  const auto hb = hand_[BLACK];
  const auto col = turn();
  //http://d.hatena.ne.jp/mclh46/20100731/1280539326
  for(int i = rep_ply_ - 4, j = 0; i >= limit_ply && j < limit_num; i-=2, ++j){
    if(now == key_[i]){
      hit_point = i;
      switch(hand_cmp(hb,hand_b_[i])){
        case HAND_IS_WIN:
          return (col == BLACK) ? HAND_IS_WIN : HAND_IS_LOSE;
          break;
        case HAND_IS_LOSE:
          return (col == BLACK) ? HAND_IS_LOSE : HAND_IS_WIN;
          break;
        case HAND_IS_SAME:
          return HAND_IS_SAME;
          break;
        default :
        return HAND_IS_UNKNOWN;
        break;
      }
    }
  }
  hit_point = -1;
  return HAND_IS_UNKNOWN;
}
int Position::move_num_debug(const bool dfpn)const{
  MoveVal *mp;
  MoveVal ma[MAX_MOVE_NUM];
  mp = ma;
  int num = 0;
  int ok_num = 0;
  if(in_checked()){
    mp = gen_move<EVASION_MOVE>(*this,mp);
  }else{
    if(!dfpn){
      mp = gen_move<CAPTURE_MOVE>(*this,mp);
      mp = gen_move<QUIET_MOVE>(*this,mp);
      mp = gen_move<DROP_MOVE>(*this,mp);
    }else{
      CheckInfo ci(*this);
      mp = gen_check(*this,mp,&ci);
    }
  }
  num = static_cast<int>(mp - ma);
  CheckInfo ci(*this);
  for(int i = 0; i < num; i++){
    if(pseudo_is_legal(ma[i].move,ci.pinned())){
      ok_num++;
    }
  }
  return ok_num;
}
bool Position::move_is_mate_with_pawn_drop(const Square sq)const{

  ASSERT(is_ok());
  //TODO const_cast
  Position * p = const_cast<Position*>(this);
  const auto me    = opp_color(p->turn());
  const auto opp   = p->turn();
  const auto ksq = p->king_sq(me);
  const auto to = ksq + ((me == BLACK) ? UP : DOWN);
  if(sq != to){
    return false;
  }
  p->operate_bb(me,to,PAWN);
  auto from_bb = p->attack_to(me,to);
  const auto pins = p->pinned_piece(me);
  //capture checking pawn
  while(from_bb.is_not_zero()){
    const auto from = from_bb.lsb<D>();
    if(p->pseudo_is_legal(me,from,to,pins)){
      p->operate_bb(me,to,PAWN);
      ASSERT(is_ok());
      return false;
    }
  }
  //capture or escape chcking pawn by king
  auto att = p->attack_from(me,KING,ksq) & (~color_bb(me));
  while(att.is_not_zero()){
    const auto escape= att.lsb<D>();
    if(!p->attack_to(opp,escape).is_not_zero()){
      p->operate_bb(me,to,PAWN);
      ASSERT(is_ok());
      return false;
    }
  }
  p->operate_bb(me,to,PAWN);
  ASSERT(is_ok());
  return true;
}
template bool Position::move_is_pseudo<BLACK>(const Move & move)const;
template bool Position::move_is_pseudo<WHITE>(const Move & move)const;

