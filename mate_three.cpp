#include "misc.h"
#include "position.h"
#include "generator.h"
#include "search_info.h"
#include "search.h"
#include "boost/utility.hpp"

#if 0

static u32 debug_flag = 0;
#define DOUT(...) if(debug_flag){ printf("col:%d ",turn());\
printf("ply:%d" ,ply());\
printf("key:%llx ",key());\
printf(__VA_ARGS__); }
#define DOUT2(...) if(debug_flag){ printf(__VA_ARGS__); }
#define OUT(...) printf(__VA_ARGS__)
#else
#define DOUT(...)
#define DOUT2(...)
#define OUT(...)
#endif

#ifdef LEARN
constexpr u64 MATE_TRANS_SHIFT = 2;
#else
constexpr u64 MATE_TRANS_SHIFT = 20;
BOOST_STATIC_ASSERT(MATE_TRANS_SHIFT >= 16);
#endif
constexpr u64 MATE_TRANS_SIZE = 1 << MATE_TRANS_SHIFT;
constexpr u64 MATE_TRANS_MASK = MATE_TRANS_SIZE - 1;

u64 mate_trans[MATE_TRANS_SIZE];

enum MateThreeEvasionPhase{ EVASION_START, CAPTURE_CHECKER_BY_KING, CAPTURE_CHECKER, CAPTURE_KING, MOVE_KING, MOVE_AIGOMA, EVASION_END };
enum_op(MateThreeEvasionPhase);

class MateThreeEvasion : boost::noncopyable{
  private:
    Move moves_[MAX_MOVE_NUM];
    Position & pos_;
    CheckInfo ci_;
    template<Color c>void next_phase();
    Move * cur_;
    Move * last_;
    Move * weak_;
    Square checker_sq_[2];
    Square ksq_;
  public:
    MateThreeEvasionPhase phase_;
    bool move_is_weak_;
    const bool in_chuai_;
    MateThreeEvasion(Position & pos, CheckInfo & ci, const bool in_chuai);
    template<Color c> Move next_move();
};
MateThreeEvasion::MateThreeEvasion(Position & pos, CheckInfo & ci, const bool in_chuai) : pos_(pos), ci_(ci), in_chuai_(in_chuai){

  ksq_ = pos.king_sq(pos.turn());
  Bitboard bb = pos.checker_bb();
  ASSERT(bb.is_not_zero());
  checker_sq_[0] = bb.lsb<D>();
  checker_sq_[1] = SQUARE_SIZE;
  phase_ = EVASION_START;
  cur_ = last_ = moves_;
  weak_ = nullptr;
  move_is_weak_ = false;
  if(bb.is_not_zero()){
    checker_sq_[1] = bb.lsb<D>();
    const Bitboard att = get_king_attack(ksq_);
    if(att.is_seted(checker_sq_[1])){
      const auto tmp = checker_sq_[0];
      checker_sq_[0] = checker_sq_[1];
      checker_sq_[1] = tmp;
    }
  }
}
template<Color c>inline Move * gen_capture_checker_by_king_move(Position & pos, Move * mlist, const Square ksq,const Square to);
template<Color c>inline Move * gen_move_to(Position & pos, Move * mlist, const Square to, const Bitboard & pins);
template<Color c, bool is_cap>inline Move * gen_king_move(Position & pos, Move * mlist, const Square from, const Square checker_sq[]);
template<Color c>inline Move * gen_aigoma_move(Position & pos, Move * mlist, const Square ksq, const Square checker_sq, const Bitboard & pins, Move ** strong_num, const bool in_chuai);

static inline Move nomate_move(){
  return move_pass();//hack
}

void init_mate_trans(){

  std::cout<<"cleaning mate trans...";
  for(u64 i = 0; i < MATE_TRANS_SIZE; ++i){
    mate_trans[i] = 0;
  }
  std::cout<<"end\n";
}

static inline void store(const Key & key, const Hand & hand_b, const Move & move){

#ifdef LEARN
  return;
#endif

  u64 entry = (key ^ (Key(hand_b.value()) << MATE_TRANS_SHIFT));
  u64 index = entry & MATE_TRANS_MASK;
  ASSERT(index < MATE_TRANS_SIZE);
  entry = entry & (~MATE_TRANS_MASK);
  const u32 store_move = move.value();
  entry = entry | u64(store_move);
  mate_trans[index] = entry;

}
static inline bool probe(const Key & key, const Hand & hand_b, Move & move){

#ifdef LEARN
  return false;
#endif

  u64 entry = (key ^ (Key(hand_b.value()) << MATE_TRANS_SHIFT));
  u64 index = entry & MATE_TRANS_MASK;
  ASSERT(index < MATE_TRANS_SIZE);
  entry = entry & (~MATE_TRANS_MASK);
  u64 word = mate_trans[index];
  if((word & (~MATE_TRANS_MASK)) == entry){
    u32 sm = u32(word & MATE_TRANS_MASK);
    move.set(sm);
    search_info.hit_mate_trans_++;
    return true;
  }
  return false;
}
template<Color c>Move Position::mate_three(CheckInfo * ci){

  ASSERT(!in_checked());
  ASSERT(is_ok());
  ASSERT(c == turn());
  MoveVal moves[128];

  if(probe(key(),hand(turn()),moves[0].move)){
    if(move_is_pseudo(moves[0].move) && pseudo_is_legal(moves[0].move,ci->pinned())){
      return moves[0].move;
    }
  }
  MoveVal *pm = moves;
  pm = gen_check<c,true>(*this,pm,ci);
  for(MoveVal *p = moves; p < pm; ++p){
    if(pseudo_is_legal(p->move,ci->pinned())){
      ASSERT(move_is_check(p->move,*ci));
      do_move(p->move,*ci,true);

      bool value = mate_three_evasion<opp_color(c)>(false);
      undo_move(p->move);
      DOUT("result %d\n",value);
      if(!value){
        store(key(),hand(turn()),p->move);
        return p->move;
      }
    }
  }
  store(key(),hand(turn()),move_none());
  return move_none();
}
template<Color c>bool Position::mate_three_evasion(const bool in_chuai){

#ifdef NDEBUG
  if(!in_checked()){
    print();
    for(int i = 0; i < ply(); i++){
      std::cout<<cur_move(i).str()<<std::endl;
    }
  }
#endif

  ASSERT(in_checked());
  ASSERT(is_ok());
  ASSERT(turn() == c);

  CheckInfo ci(*this);
  MateThreeEvasion mte(*this,ci,in_chuai);
  Move move;
  while((move = mte.next_move<c>()) != move_none()){
    if(move == nomate_move()){
      DOUT("yaneura no mate\n");
      return true;
    }
    DOUT("evasion %s\n",move.str().c_str());
    if(pseudo_is_legal(move,ci.pinned())){
      bool check = move_is_check(move,ci);
      do_move(move,ci,check);
      ASSERT(!check_in_checked(c));
      Move ret(0);
      if(mte.phase_ == MOVE_AIGOMA){
        ret = recap_move<opp_color(c)>();
        DOUT("try recap result %s\n",ret.str(*this).c_str());
      }
      if(!check && !ret.value()){
        ASSERT(!check_in_checked(c));
        ret = mate_one<opp_color(c)>();
        DOUT("try mate one result %s\n",ret.str(*this).c_str());
      }
      undo_move(move);
      DOUT("evasion result %s\n",ret.str(*this).c_str());
      if(!ret.value()){
        return true;
      }
    }
  }
  DOUT("evasion loop end\n");
  return false;
}
template<Color c>Move Position::recap_move(){

  ASSERT(is_ok());
  ASSERT(turn() == c);
  ASSERT(checker_bb(ply()-1).is_not_zero());

  if(ply() > MAX_PLY - 10){
    return move_none();
  }
  const auto from = checker_bb(ply()-1).lsb<N>();
  const auto to = cur_move(ply()-1).to();
  const auto rank = square_to_rank(to);
  const auto pc = piece_to_piece_type(square(from));
  Move recap;
  if((is_prom_square<c>(from) || is_prom_square<c>(to))
      && ((pc == ROOK || pc == BISHOP) || (pc == LANCE && is_include_rank<c,RANK_2>(rank)))){
    recap = make_prom_move(from,to);
  }else{
    recap = make_move(from,to);
  }
  DOUT("recap move is %s\n",recap.str().c_str());
  CheckInfo ci(*this);
  //const auto me = turn();
  constexpr auto me = c;
  if(move_is_pseudo(recap) && pseudo_is_legal(recap,ci.pinned())){
    DOUT("recap domove\n");
    do_move(recap,ci,true);
    //打つ手じゃない場合は逆王手の可能性がある
    if(!cur_move(ply()-1).is_drop()){
      if(check_in_checked(me)){
        undo_move(recap);
        DOUT("recap do but checked\n");
        return move_none();
      }
    }
    bool value = mate_three_evasion<opp_color(c)>(true);
    undo_move(recap);
    if(value){
      return move_none();
    }
    return recap;
  }
  return move_none();
}
template<Color c>void MateThreeEvasion::next_phase(){

  cur_ = last_ =  moves_;
  switch(++phase_){
    case CAPTURE_CHECKER_BY_KING:
      last_ = gen_capture_checker_by_king_move<c>(pos_,moves_,ksq_,checker_sq_[0]);
      DOUT2("gen_cap checker king %d\n",int(last_ - moves_));
      return;
      break;
    case CAPTURE_CHECKER:
      if(checker_sq_[1] == SQUARE_SIZE){
        last_ = gen_move_to<c>(pos_,moves_,checker_sq_[0],ci_.pinned());
        DOUT2("gen_cap checker %d\n",int(last_ - moves_));
      }
      return;
      break;
    case CAPTURE_KING:
      last_ = gen_king_move<c,true>(pos_,moves_,ksq_,checker_sq_);
      DOUT2("gen_cap king %d\n",int(last_ - moves_));
      return;
      break;
    case MOVE_KING:
      last_ = gen_king_move<c,false>(pos_,moves_,ksq_,checker_sq_);
      DOUT2("gen_king %d\n",int(last_ - moves_));
      return;
      break;
    case MOVE_AIGOMA:
      if(checker_sq_[1] == SQUARE_SIZE){
        ASSERT(weak_ == nullptr);
        last_ = gen_aigoma_move<c>(pos_,moves_,ksq_,checker_sq_[0],ci_.pinned(),&weak_,in_chuai_);
        DOUT2("gen_aigoma %d\n",int(last_ - moves_));
        DOUT2("gen_aigoma strong %d\n",int(weak_ - moves_));
      }
      return;
      break;
    case EVASION_END:
      last_ = cur_ + 1;
      return;
    default:
      std::cout<<phase_<<std::endl;
      ASSERT(false);
      return;
  }

}
template<Color c>Move MateThreeEvasion::next_move(){

  Move move;
  while(true){
    while(cur_ == last_){
      next_phase<c>();
    }
    switch(phase_){
      case CAPTURE_CHECKER_BY_KING : case CAPTURE_CHECKER :
      case CAPTURE_KING : case MOVE_KING :
        move = *cur_;
        ++cur_;
        DOUT2("next move phase = %d %s\n",phase_,move.str().c_str());
        return move;
        break;
      case MOVE_AIGOMA:
        if(weak_ == nullptr){
          DOUT2("yaneura nomate_move\n");
          return nomate_move();
        }
        else{
          move_is_weak_ = cur_ >= weak_;
          DOUT2("move is weak %d\n",move_is_weak_);
        }
        move = *cur_;
        ++cur_;
        DOUT2("next move phase = %d %s\n",phase_,move.str().c_str());
        return move;
        break;
      case EVASION_END:
        return move_none();
      default:
        ASSERT(false);
        break;
    }
  }
  ASSERT(false);
  return move_none();

}
//王手してる駒を王で取る手
template<Color c>inline Move * gen_capture_checker_by_king_move(Position & pos, Move * mlist, const Square ksq,const Square to){

  ASSERT(pos.square(to) != EMPTY);
  ASSERT(piece_color(pos.square(to)) != c);
  const Bitboard katt = get_king_attack(ksq);
  if(!katt.is_seted(to)){
    return mlist;
  }
  if(pos.attack_to(opp_color(c),to).is_not_zero()){
    return mlist;
  }
  (*mlist++) = make_move(ksq,to);
  return mlist;
}
//toに行く手を生成する
template<Color c>inline Move * gen_move_to(Position & pos, Move * mlist, const Square to, const Bitboard & pins){

  const Rank to_rank = square_to_rank(to);
  const bool is_prom  = (c == BLACK) ? (to_rank <= RANK_3)
    : (to_rank >= RANK_7);
  const bool is_rank2 = (c == BLACK) ? (to_rank <= RANK_2)
    : (to_rank >= RANK_8);
  const Bitboard cap = pos.attack_to(c,to);
  Bitboard piece;
  //なるべく弱い駒から王手したほうがいいと思う
  //pawn
  piece = pos.piece_bb(PAWN) & cap;
  while(piece.is_not_zero()){
    const auto from = piece.lsb<D>();
    if(pos.pseudo_is_legal(c,from,to,pins)){
      if(is_prom){
        (*mlist++) = make_prom_move(from,to);
      }else{
        (*mlist++) = make_move(from,to);
      }
    }
  }
  //knight,lance
  piece = (pos.piece_bb(KNIGHT) | pos.piece_bb(LANCE)) & cap;
  while(piece.is_not_zero()){
    const auto from = piece.lsb<D>();
    if(pos.pseudo_is_legal(c,from,to,pins)){
      if(is_rank2){//成る手だけ
        (*mlist++) = make_prom_move(from,to);
      }else if(is_prom){//成る手と成らない手
        (*mlist++) = make_prom_move(from,to);
        (*mlist++) = make_move(from,to);
      }else{//成らない手
        (*mlist++) = make_move(from,to);
      }
    }
  }
  //silver
  piece = pos.piece_bb(SILVER) & cap;
  while(piece.is_not_zero()){
    const auto from = piece.lsb<D>();
    if(pos.pseudo_is_legal(c,from,to,pins)){
      if(is_prom_square<c>(from) || is_prom){
        (*mlist++) = make_prom_move(from,to);
      }
      (*mlist++) = make_move(from,to);
    }
  }
  //rook,bishop
  piece = (pos.piece_bb(ROOK) | pos.piece_bb(BISHOP)) & cap;
  while(piece.is_not_zero()){
    const auto from = piece.lsb<D>();
    if(pos.pseudo_is_legal(c,from,to,pins)){
      if(is_prom_square<c>(from) || is_prom){
        (*mlist++) = make_prom_move(from,to);
      }else{
        (*mlist++) = make_move(from,to);
      }
    }
  }
  //prook,pbishop,golds
  piece =  (pos.piece_bb(PROOK) | pos.piece_bb(PBISHOP) | pos.golds_bb()) & cap;
  while(piece.is_not_zero()){
    const auto from = piece.lsb<D>();
    if(pos.pseudo_is_legal(c,from,to,pins)){
      (*mlist++) = make_move(from,to);
    }
  }
  return mlist;
}
//玉が逃げる手
template<Color c, bool is_cap>inline Move * gen_king_move(Position & pos, Move * mlist, const Square from, const Square checker_sq[]){

  Bitboard att = get_king_attack(from);
  if(is_cap){
    att &= pos.color_bb(opp_color(c));
    att &= (~mask[checker_sq[0]]);
  }else{
    att &= ~pos.occ_bb();
  }
  while(att.is_not_zero()){
    const auto to = att.lsb<D>();
    if(checker_sq[1] != SQUARE_SIZE){
      const SquareRelation r1 = get_square_relation(from,checker_sq[1]);
      const SquareRelation r2 = get_square_relation(from,to);
      if(r1 == r2){
        continue;
      }
    }
    if(checker_sq[0] != to){//TODO need ?
      const SquareRelation r1 = get_square_relation(from,checker_sq[0]);
      const SquareRelation r2 = get_square_relation(from,to);
      const PieceType pt = piece_to_piece_type(pos.square(checker_sq[0]));
      if(r1 == r2){
        switch(r1){
          case RANK_RELATION:
            if(pt == ROOK || pt == PROOK){
              continue;
            }
            break;
          case FILE_RELATION:
            if(pt == ROOK || pt == PROOK || pt == LANCE){
              continue;
            }
            break;
          case LEFT_UP_RELATION: case RIGHT_UP_RELATION:
            if(pt == BISHOP || pt == PBISHOP){
              continue;
            }
            break;
          default:
            break;
        }
      }
    }
    if(!pos.attack_to(opp_color(c),to).is_not_zero()){
      (*mlist++) = make_move(from,to);
    }
  }
  return mlist;
}
template<Color c>
inline Move * gen_aigoma_move
(Position & pos, Move * mlist, const Square ksq, const Square checker_sq, const Bitboard & pins, Move **strong_num, const bool in_chuai){

  const SquareRelation sr = get_square_relation(ksq, checker_sq);
  Square inc;
  Move moves[64];//作業領域
  Move *sp;
  Move chuai_moves[64];//弱中合いの手を入れる
  Move *chuai_sp = chuai_moves;
  int min_chuai_distance = INT_MAX;

  switch(sr){
    case FILE_RELATION:
      {
        inc = (ksq > checker_sq) ? UP : DOWN;
        const auto kfile = square_to_file(checker_sq);
        min_chuai_distance = ((kfile == FILE_A) || (kfile == FILE_I)) ? 2 : 4;
        break;
      }
    case RANK_RELATION:
      {
        inc = (ksq > checker_sq) ? RIGHT: LEFT;
        const auto krank = square_to_rank(checker_sq);
        min_chuai_distance = ((krank == RANK_1) || (krank == RANK_9)) ? 2 : 4;
        break;
      }
    case LEFT_UP_RELATION:
      inc = (ksq > checker_sq) ? RIGHT_DOWN : LEFT_UP;
      min_chuai_distance = 3;
      break;
    case RIGHT_UP_RELATION:
      inc = (ksq > checker_sq) ? RIGHT_UP : LEFT_DOWN;
      min_chuai_distance = 3;
      break;
    default://桂馬の王手は合い駒できない
      return mlist;
      break;
  }
  ASSERT(min_chuai_distance != INT_MAX);
  const Hand hand = pos.hand(c);
  const Bitboard pawn_bb = pos.piece_bb(c,PAWN);
  int distance = 1;
  for(Square to = ksq + inc; to != checker_sq; to += inc, ++distance){
    //move on square
    sp = moves;
    sp = gen_move_to<c>(pos,sp,to,pins);
    int attack_num = static_cast<int>(sp - moves);
    if(to == ksq + inc){//玉のとなりなら玉の利きを加える
      ++attack_num;
    }
    if(attack_num > 1){//ちゅうあいじゃない
      for(Move * p = moves; p != sp; ++p){
        (*mlist++) = *p;
      }
    }
    else if(sp != moves){//このあとの合いゴマは中合いじゃないけどこのては中合い
      ASSERT(attack_num == 1);
      (*chuai_sp++) = moves[0];
    }else{//中合い
      //手はない
      ASSERT(attack_num == 0 || (attack_num == 1 && to == ksq + inc));
      ASSERT(sp == moves);
      if(in_chuai){
        ASSERT(pos.ply() >= 2);
        if(pos.cur_move(pos.ply()-2).is_drop()){
          DOUT2("やねうらおヒューリスティックno1\n");
          //printf("やねうらおヒューリスティックno1\n");
          continue;
        }
      }
      if(pos.cur_move(pos.ply()-1).to() == checker_sq
      && distance > min_chuai_distance){
        DOUT2("やねうらおヒューリスティックno2\n");
        //printf("やねうらおヒューリスティックno2\n");
        continue;
      }
    }
    sp = moves;
    //drop
    const File file = square_to_file(to);
    const Rank rank = square_to_rank(to);
    const bool double_pawn = (pawn_bb & file_mask[file]).is_not_zero();
    const bool is_rank3 = c ? (rank <= RANK_7)
                            : (rank >= RANK_3);
    const bool is_rank2 = c ? (rank <= RANK_8)
                            : (rank >= RANK_2);
    //弱い駒から生成する
    //http://d.hatena.ne.jp/LS3600/20100115/p1
    //王手回避の駒打ちは，「飛・香・歩」の順番で，可能なものを１つだけ選択し
    //ます．しかし中合の場合は，どうせ取られるので「歩・香・飛」の順番で１つ
    //選択すべきです．
    if(hand.is_have(PAWN_H)
        && !double_pawn
        && is_rank2
        && !pos.move_is_mate_with_pawn_drop(to)){
      (*sp++) = make_drop_move(to,PAWN_H);
    }
    if(hand.is_have(LANCE_H) && is_rank2){
      (*sp++) = make_drop_move(to,LANCE_H);
    }
    if(hand.is_have(KNIGHT_H) && is_rank3){
      (*sp++) = make_drop_move(to,KNIGHT_H);
    }
    if(hand.is_have(SILVER_H)){
      (*sp++) = make_drop_move(to,SILVER_H);
    }
    if(hand.is_have(GOLD_H)){
      (*sp++) = make_drop_move(to,GOLD_H);
    }
    if(hand.is_have(BISHOP_H)){
      (*sp++) = make_drop_move(to,BISHOP_H);
    }
    if(hand.is_have(ROOK_H)){
      (*sp++) = make_drop_move(to,ROOK_H);
    }
    if(attack_num){//中合いじゃない
      if(sp != moves && distance > min_chuai_distance
      && pos.cur_move(pos.ply()-1).is_drop()){
        DOUT2("やねうらおヒューリスティックno3\n");
        printf("やねうらおヒューリスティックno3\n");
        *strong_num = nullptr;
        return mlist + 1;//HACK to skip next_phase
      }
      for(Move * p = moves; p != sp; ++p){
        (*mlist++) = *p;
      }
    }else{//中合い
      for(Move * p = moves; p != sp; ++p){
        (*chuai_sp++) = *p;
      }
    }
  }
  *strong_num = mlist;
  //中合いの手をお尻にくっつける
  for(Move * p = chuai_moves; p != chuai_sp; ++p){
    (*mlist++) = *p;
  }
  return mlist;
}


template Move Position::mate_three<BLACK>(CheckInfo * ci);
template Move Position::mate_three<WHITE>(CheckInfo * ci);

template bool Position::mate_three_evasion<BLACK>(const bool in_chuai);
template bool Position::mate_three_evasion<WHITE>(const bool in_chuai);

