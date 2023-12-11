#ifndef POSITION_H
#define POSITION_H

#include "type.h"
#include "piece.h"
#include "evaluator.h"
#include "bitboard.h"
#include "hand.h"
#include "move.h"
#include "pv.h"
#include "check_info.h"
#include <string>
#include <limits.h>
#include "boost/utility.hpp"
#include <boost/thread/thread.hpp>

constexpr int TLP_MAX_THREAD = 12;
constexpr int TLP_NUM_WORK = TLP_MAX_THREAD * 8 ;

class Sort;

extern Key singular_key_seed;

struct KillerMove{
  static const int NUM = 2;
  void init(){
    for(int i = 0; i < NUM; i++){
      move_[i] = move_none();
    }
  }
  void update(const Move & m){
    if(m != move_[0]){
      move_[1] = move_[0];
      move_[0] = m;
    }
  }
  Move move_[NUM];
};

class Position{
  private:
    static const int MAX_REP = 1024;

  public:
  //コンストラクタ
    Position(){}
    Position(const Piece array[], const Color turn, const Hand & hand_b, const Hand & hand_w){
      init_position(array,turn,hand_b,hand_w);
    }
    Position(const std::string buf){
      init_position(buf);
    }
    Position& operator = (const Position & pos){
      std::memcpy(this,&pos,sizeof(Position));
      return *this;
    }
  //初期化
    bool init_position();
    void init_position(const Position & p);
    bool init_position(const std::string s);
    bool init_position(const Piece array[],const Color turn, const Hand & hand_b, const Hand & hand_w);
    void init_hirate();
    inline void init_node(){
      node_num_ = 0ULL;
    }
    inline void init_pv(){
      pv_.init(ply_);
    }
  //アクセッサ
    inline Piece square(const Square xy)const{
      ASSERT(square_is_ok(xy));
      return square_[xy];
    }
    inline Bitboard color_bb(const Color c)const{
      ASSERT(color_is_ok(c));
      return color_bb_[c];
    }
    inline Bitboard piece_bb(const PieceType p)const{
      ASSERT(piece_type_is_ok(p));
      return piece_bb_[p];
    }
    inline Bitboard piece_bb(const Color c, const PieceType p)const{
      ASSERT(color_is_ok(c));
      ASSERT(piece_type_is_ok(p));
      return color_bb_[c] & piece_bb_[p];
    }
    inline Bitboard piece_bb(const PieceType p1, const PieceType p2)const{
      ASSERT(piece_type_is_ok(p1));
      ASSERT(piece_type_is_ok(p2));
      return piece_bb_[p1] | piece_bb_[p2];
    }
    inline Bitboard piece_bb(const Color c, const PieceType p1, const PieceType p2)const{
      ASSERT(color_is_ok(c));
      ASSERT(piece_type_is_ok(p1));
      ASSERT(piece_type_is_ok(p2));
      return piece_bb(p1,p2) & color_bb(c);
    }
    inline Bitboard piece_bb(const PieceType p1, const PieceType p2, const PieceType p3)const{
      ASSERT(piece_type_is_ok(p1));
      ASSERT(piece_type_is_ok(p2));
      ASSERT(piece_type_is_ok(p3));
      return piece_bb(p1,p2) | piece_bb_[p3];
    }
    inline Bitboard piece_bb(const PieceType p1, const PieceType p2, const PieceType p3, const PieceType p4)const{
      ASSERT(piece_type_is_ok(p1));
      ASSERT(piece_type_is_ok(p2));
      ASSERT(piece_type_is_ok(p3));
      ASSERT(piece_type_is_ok(p4));
      return piece_bb(p1,p2,p3) | piece_bb_[p4];
    }
    inline Bitboard piece_bb(const PieceType p1, const PieceType p2, const PieceType p3, const PieceType p4, const PieceType p5)const{
      ASSERT(piece_type_is_ok(p1));
      ASSERT(piece_type_is_ok(p2));
      ASSERT(piece_type_is_ok(p3));
      ASSERT(piece_type_is_ok(p4));
      ASSERT(piece_type_is_ok(p5));
      return piece_bb(p1,p2,p3,p4) | piece_bb_[p5];
    }
    inline Bitboard piece_bb(const Piece p)const{
      ASSERT(piece_is_ok(p));
      const Color c = piece_color(p);
      const PieceType pt = piece_to_piece_type(p);
      ASSERT(color_is_ok(c));
      ASSERT(piece_type_is_ok(pt));
      return color_bb_[c] & piece_bb_[pt];
    }
    inline Bitboard golds_bb()const{
      return piece_bb(GOLD,PPAWN,PLANCE,PKNIGHT,PSILVER);
    }
    inline Bitboard golds_bb(const Color c)const{
      ASSERT(color_is_ok(c));
      return (golds_bb()) & color_bb(c);
    }
    inline Bitboard occ_bb()const{
      return piece_bb_[OCCUPIED];
    }
    inline Color turn()const{
      return turn_;
    }
    inline Square king_sq(const Color c)const{
      //ASSERT(piece_bb_[KING].pop_cnt() == 2);
      ASSERT(color_is_ok(c));
      return (color_bb_[c] & piece_bb_[KING]).lsb<N>();
    }
    inline Hand hand(const Color c)const{
      ASSERT(color_is_ok(c));
      return hand_[c];
    }
    Key key()const{
      return key_[rep_ply_];
    }
    Key key(const int p)const{
      return key_[p];
    }
    Key singular_key() const {
      return key_[rep_ply_] ^ singular_key_seed;
    }
    Value material(const int p)const{
      return material_[p];
    }
    Value material()const{
      return material(ply());
    }
    Value eval()const{
      return eval(ply());
    }
    Value eval(const int p)const{
      ASSERT(p >= 0);
      ASSERT(p < MAX_PLY);
      return eval_[p];
    }
    bool eval_is_empty(const int p)const{
      return eval_[p] == STORED_EVAL_IS_EMPTY;
    }
    bool eval_is_empty()const{
      return eval_is_empty(ply());
    }
    int ply()const{
      return ply_;
    }
    Piece capture(const int p)const{
      return capture_[p];
    }
    Piece capture()const{
      return capture_[ply()];
    }
    Bitboard checker_bb(const int p)const{
      return checker_bb_[p];
    }
    Bitboard checker_bb()const{
      return checker_bb(ply());
    }
    Hand hand_b()const{
      return hand_b_[rep_ply_];
    }
    Hand hand_b(const int p)const{
      return hand_b_[p];
    }
    inline u64 node_num()const{
      return node_num_;
    }
    Move cur_move(const int p)const{
      return cur_move_[p];
    }
    Move skip_move(const int p)const{
      return skip_move_[p];
    }
    inline Move pv(const int ply, const int ply2)const{
      return pv_.move(ply,ply2);
    }
    inline Pv* pv(){
      return &pv_;
    }
    inline Pv pv2()const{
      return pv_;
    }
    Move trans_move(const int p)const{
      return trans_move_[p];
    }
    Move trans_move()const{
      return trans_move(ply());
    }
    Depth reduction(const int p)const{
      return reduction_[p];
    }
  //ミューテータ
    inline void set_turn(const Color c){
      ASSERT(color_is_ok(c));
      turn_ = c;
    }
    inline void set_square(const Piece p, const Square xy){
      ASSERT(p == EMPTY || piece_is_ok(p));
      ASSERT(square_is_ok(xy));
      square_[xy] = p;
    }
    inline void set_xor_bb(const Color c, const Square xy){
      ASSERT(color_is_ok(c));
      ASSERT(square_is_ok(xy));
      color_bb_[c].Xor(xy);
    }
    inline void set_xor_bb(const PieceType pt, const Square xy){
      ASSERT(pt == OCCUPIED ||piece_type_is_ok(pt));
      ASSERT(square_is_ok(xy));
      piece_bb_[pt].Xor(xy);
    }
    void set_key(const Key k){
      key_[rep_ply_] = k;
    }
    void set_material(const Value v){
      material_[ply_] = v;
    }
    void set_eval(const Value v, const int p){
      eval_[p] = v;
    }
    void set_eval(const Value v){
      set_eval(v,ply());
    }
    void set_ply(const int p){
      ply_ = p;
    }
    void set_capture(Piece p){
      capture_[ply_] = p;
    }
    void set_checker_bb(const Bitboard & bb, const int p){
      checker_bb_[p] = bb;
    }
    void set_checker_bb(const Bitboard & bb){
      set_checker_bb(bb,ply_+1);
    }
    void set_hand_b(const Hand & h){
      hand_b_[rep_ply_] = h;
    }
    void set_cur_move(const int p, const Move & m){
      cur_move_[p] = m;
    }
    void set_trans_move(const Move & move){
      trans_move_[ply()] = move;
    }
    void set_reduction(const int p, const Depth r){
      reduction_[p] = r;
    }
    void set_reduction(const Depth r){
      set_reduction(ply(),r);
    }
    void set_skip_move(const int p, const Move & m){
      skip_move_[p] = m;
    }
    void set_pv(const Move & m, const int p, const int p2){
      pv_.set_move(m,p,p2);
    }
  //メソッド
  //position系
    inline void inc_hand(const HandPiece hp, const Color c){
      ASSERT(hand_piece_is_ok(hp));
      ASSERT(color_is_ok(c));
      hand_[c].inc(hp);
    }
    inline void dec_hand(const HandPiece hp, const Color c){
      ASSERT(color_is_ok(c));
      ASSERT(hand_piece_is_ok(hp));
      hand_[c].dec(hp);
    }
    void inc_ply(){
      ++ply_;
      ASSERT(ply_is_ok(ply_));
    }
    void dec_ply(){
      --ply_;
      ASSERT(ply_is_ok(ply_));
    }
    bool in_checked(const int p)const{
      return checker_bb(p).is_not_zero();
    }
    bool in_checked()const{
      const bool ret = in_checked(ply());
#ifndef NDEBUG
      const auto opp = opp_color(turn());
      const auto ksq = king_sq(turn());
      const bool check = attack_to(opp,ksq).is_not_zero();
      if(check != ret){
        dump();
        ASSERT(false);
      }
#endif
      return ret;
    }
    bool check_in_checked(const Color c)const{
      const auto opp = opp_color(c);
      const auto ksq = king_sq(c);
      return attack_to(opp,ksq).is_not_zero();
    }
    Bitboard hidden_checker(const Square king_sq,
        const Color checker_color, const Color pinned_color)const;

    inline Bitboard pinned_piece(const Color pinned_color)const{
      ASSERT(color_is_ok(pinned_color));
      //ASSERT(is_ok());
      return hidden_checker(king_sq(pinned_color),
          opp_color(pinned_color),
          pinned_color);
    }
    inline Bitboard discover_checker()const{
      //ASSERT(is_ok());
      return hidden_checker(king_sq(opp_color(turn())),
          turn(),
          turn());
    }
    inline void update_pv(const Move & move){
      pv_.update(move,ply_);
    }
    void update_pv(const Move & move,Position * child){
      pv_.update(move,ply_,child->pv());
    }
    inline void inc_node(){
      ++node_num_;
    }
    inline void inc_node(const u64 n){
      node_num_ += n;
    }
    void do_move(const Move & move, const CheckInfo & ci, const bool check);
    void do_move(const Move & move, const bool check){
      CheckInfo ci(*this);
      do_move(move,ci,check);
    }
    void undo_move(const Move & move);
    void do_null_move();
    void undo_null_move();
    template<class Func>void do_undo_move(const Move & move, const CheckInfo & ci, const bool check, Func & f);
    template <PieceType p> inline Bitboard attack_from(const Color c, const Square from)const;
    template <Color c, PieceType p> inline Bitboard attack_from(const Square from)const;
    inline Bitboard attack_from(const Color c, const PieceType p, const Square from)const;
    inline Bitboard attack_from(const Piece p, const Square from)const;
    inline Bitboard attack_from(const Color c, const PieceType p, const Square from, const Bitboard & occ)const;
    inline Bitboard attack_from(const Piece p, const Square from, const Bitboard & occ)const;
    template <Piece p> inline Bitboard attack_from(const Square from)const;
    template <Color c, PieceType p> inline Bitboard attack_from(const Square from, Bitboard & occ)const;
    template <Piece p> Bitboard inline attack_from(const Square from, const Bitboard & occ)const;
    inline Bitboard attack_to(const Color c, const Square sq)const;
    inline Bitboard attack_to(const Square sq)const;
    inline Bitboard attack_to(const Square sq, const Bitboard & occ)const;
    template<int pt>inline Bitboard attack_all(const Color c)const;
    HandState check_rep(const int limit_ply, const int limit_num, int & hit_point)const;

    //move系
    bool move_is_check(const Move & move, const CheckInfo & ci)const;
    bool move_is_pseudo(const Move & move)const{
      return turn() ? move_is_pseudo<WHITE>(move)
        : move_is_pseudo<BLACK>(move);
    }
    template<Color c>bool move_is_pseudo(const Move & move)const;
    bool move_is_mate_with_pawn_drop(const Square to)const;
    bool pseudo_is_legal(const Color c, const Square from,
        const Square to, const Bitboard & pinned)const;
    bool pseudo_is_legal(const Square from, const Square to, const Bitboard & pinned)const;
    bool pseudo_is_legal(const Move & move, const Bitboard & pinned)const;
    bool move_is_tactical(const Move & move)const{
      const auto from = move.from();
      if(square_is_drop(from)){
        return false;
      }
      const auto to = move.to();
      const auto cap = square(to);
      if(cap){
        return true;
      }
      const auto pt = piece_to_piece_type(square(from));
      const auto prom = move.is_prom();
      return (prom && (pt != SILVER));
    }
    bool move_is_discover(const Square from, const Square to, const Bitboard & discover_checker) const {
      const auto opp = opp_color(turn());
      const auto ksq = king_sq(opp);
      const auto fk_sr = get_square_relation(from,ksq);
      return (fk_sr
          && fk_sr != get_square_relation(from,to)
          && discover_checker.is_seted(from));
    }
    Value see(const Move & move, const Value root_alpha, const Value root_beta)const;
    Value mvv_lva(const Move & move)const{
      const auto from = move.from();
      const auto to = move.to();
      const auto piece = piece_to_piece_type(square(from));
      const auto cap = piece_to_piece_type(square(to));
      return (piece_type_value_ex(cap) * 256)
        - piece_type_value_ex(piece) ;
    }
    //mate系
    template<Color c>Move mate_one(CheckInfo & ci);
    Move mate_one(CheckInfo & ci){
      return turn() ? mate_one<WHITE>(ci)
        : mate_one<BLACK>(ci);
    }
    template<Color c>Move mate_one(){
      ASSERT(is_ok());
      CheckInfo ci(*this);
      Move move = mate_one<c>(ci);
      ASSERT(!move.value()||move.is_ok(*this));
      ASSERT(is_ok());
      ASSERT(!move.value() || mate_one_is_ok(move));
      return move;
    }
    Move mate_one(){
      return turn() ? mate_one<WHITE>()
        : mate_one<BLACK>();
    }
    template<Color c>Move mate_three(CheckInfo * ci);
    template<Color c>Move mate_three(){
      ASSERT(is_ok());
      CheckInfo ci(*this);
      Move move = mate_three<c>(&ci);
      ASSERT(!move.value()||move.is_ok(*this));
      ASSERT(is_ok());
      return move;
    }
    Move mate_three(CheckInfo * ci){
      return turn() ? mate_three<WHITE>(ci)
        : mate_three<BLACK>(ci);
    }
    Move mate_three(){
      return turn() ? mate_three<WHITE>()
        : mate_three<BLACK>();
    }
    //出力系
    void print()const;
    std::string out_sfen()const;
    //デバッグ系
    void dump()const;
    bool is_ok()const;
    bool is_ok(const Move move)const;
    int move_num_debug(const bool dfpn)const;
    //変数
    KillerMove killer_[MAX_PLY];
    int rep_ply_;//初手から今まで
    bool skip_null_;
  private:
    //メソッド
    Bitboard make_checker_bb_debug()const;
    //mate three系
    template<Color c>bool mate_three_evasion(const bool in_chuai);
    template<Color c>Move recap_move();
    bool move_is_aigoma(const Move & move)const;
    //mate one系
    template<Color c, bool have_rook, bool have_bishop, bool have_gold>Move mate_one(const CheckInfo & ci);
    void operate_bb(const Color c, const Square sq, const PieceType pt);
    bool can_escape(const Color c, const Square ksq, const Square checker, const bool is_double_check);
    bool mate_one_is_ok(const Move & move);

    u64 make_key()const;
    
    Bitboard color_bb_[COLOR_SIZE];
    Bitboard piece_bb_[PIECE_TYPE_SIZE];
    Piece square_[SQUARE_SIZE];
    Hand hand_[COLOR_SIZE];
    //stack
    Pv pv_;
    Hand hand_b_[MAX_REP];
    Key key_[MAX_REP];
    Bitboard checker_bb_[MAX_PLY];
    Piece capture_[MAX_PLY];
    Move cur_move_[MAX_PLY];
    Move skip_move_[MAX_PLY];
    Move trans_move_[MAX_PLY];
    Value material_[MAX_PLY];
    Value eval_[MAX_PLY];
    Depth reduction_[MAX_PLY];
    Color turn_;
    u64 node_num_;
    int ply_;
  public:
    Sort * psort_[MAX_PLY];
    CheckInfo * pci_[MAX_PLY];
    Position * tlp_sibling_[TLP_MAX_THREAD];
    Position * tlp_parent_;
    boost::mutex tlp_lock;
    volatile bool tlp_stop_;
    volatile bool tlp_used_;
    int tlp_slot_;
    Value tlp_alpha_;
    Value tlp_beta_;
    Value tlp_best_value_;
    Move tlp_best_move_;
    volatile int tlp_sibling_num_;
    Depth tlp_depth_;
    int tlp_id_;
    bool tlp_cut_node_;
    NodeType tlp_node_type_;
};

template <PieceType p> inline Bitboard Position::attack_from(const Color c, const Square from)const{

  ASSERT(color_is_ok(c));
  ASSERT(square_is_ok(from));
  ASSERT(piece_type_is_ok(p));
  switch(p){
    case PAWN:
      return get_pawn_attack(c,from);
    case KNIGHT:
      return get_knight_attack(c,from);
    case LANCE:
      return get_lance_attack(c,from,occ_bb());
    case SILVER:
      return get_silver_attack(c,from);
    case GOLD:
    case PPAWN:
    case PLANCE:
    case PKNIGHT:
    case PSILVER:
    case GOLDS:
      return get_gold_attack(c,from);
    case KING:
      return get_king_attack(from);
    case BISHOP:
      return get_bishop_attack(from,occ_bb());
    case ROOK:
      return get_rook_attack(from,occ_bb());
    case PBISHOP:
      return get_pbishop_attack(from,occ_bb());
    case PROOK:
      return get_prook_attack(from,occ_bb());
    default:
      ASSERT(false);
      return Bitboard(0,0);
  }
}
template <Color c, PieceType p> inline Bitboard Position::attack_from(const Square from)const{

  ASSERT(color_is_ok(c));
  ASSERT(square_is_ok(from));
  ASSERT(piece_type_is_ok(p));
  switch(p){
    case PAWN:
      return get_pawn_attack(c,from);
    case KNIGHT:
      return get_knight_attack(c,from);
    case LANCE:
      return get_lance_attack(c,from,occ_bb());
    case SILVER:
      return get_silver_attack(c,from);
    case GOLD:
    case PPAWN:
    case PLANCE:
    case PKNIGHT:
    case PSILVER:
    case GOLDS:
      return get_gold_attack(c,from);
    case KING:
      return get_king_attack(from);
    case BISHOP:
      return get_bishop_attack(from,occ_bb());
    case ROOK:
      return get_rook_attack(from,occ_bb());
    case PBISHOP:
      return get_pbishop_attack(from,occ_bb());
    case PROOK:
      return get_prook_attack(from,occ_bb());
    default:
      ASSERT(false);
      return Bitboard(0,0);
  }
}
template <Piece p> Bitboard inline Position::attack_from(const Square from)const{

  ASSERT(square_is_ok(from));
  ASSERT(piece_is_ok(p));
  switch(p){
    case PAWN_B:
      return get_pawn_attack(BLACK,from);
    case PAWN_W:
      return get_pawn_attack(WHITE,from);
    case KNIGHT_B:
      return get_knight_attack(BLACK,from);
    case KNIGHT_W:
      return get_knight_attack(WHITE,from);
    case SILVER_B:
      return get_silver_attack(BLACK,from);
    case SILVER_W:
      return get_silver_attack(WHITE,from);
    case GOLD_B:
    case PPAWN_B:
    case PLANCE_B:
    case PKNIGHT_B:
    case PSILVER_B:
      return get_gold_attack(BLACK,from);
    case GOLD_W:
    case PPAWN_W:
    case PLANCE_W:
    case PKNIGHT_W:
    case PSILVER_W:
      return get_gold_attack(WHITE,from);
    case KING_B:
    case KING_W:
      return get_king_attack(from);
    case ROOK_B:
    case ROOK_W:
      return get_rook_attack(from,occ_bb());
    case BISHOP_B:
    case BISHOP_W:
      return get_bishop_attack(from,occ_bb());
    case PROOK_B:
    case PROOK_W:
      return get_prook_attack(from,occ_bb());
    case PBISHOP_B:
    case PBISHOP_W:
      return get_pbishop_attack(from,occ_bb());
    case LANCE_B:
      return get_lance_attack(BLACK,from,occ_bb());
    case LANCE_W:
      return get_lance_attack(WHITE,from,occ_bb());
    default:
      ASSERT(false);
      return Bitboard(0,0);
  }
}
template <Piece p> inline Bitboard Position::attack_from(const Square from, const Bitboard & occ)const{

  ASSERT(square_is_ok(from));
  ASSERT(piece_is_ok(p));
  switch(p){
    case PAWN_B:
      return get_pawn_attack(BLACK,from);
    case PAWN_W:
      return get_pawn_attack(WHITE,from);
    case KNIGHT_B:
      return get_knight_attack(BLACK,from);
    case KNIGHT_W:
      return get_knight_attack(WHITE,from);
    case SILVER_B:
      return get_silver_attack(BLACK,from);
    case SILVER_W:
      return get_silver_attack(WHITE,from);
    case GOLD_B:
    case PPAWN_B:
    case PLANCE_B:
    case PKNIGHT_B:
    case PSILVER_B:
      return get_gold_attack(BLACK,from);
    case GOLD_W:
    case PPAWN_W:
    case PLANCE_W:
    case PKNIGHT_W:
    case PSILVER_W:
      return get_gold_attack(WHITE,from);
    case KING_B:
    case KING_W:
      return get_king_attack(from);
    case ROOK_B:
    case ROOK_W:
      return get_rook_attack(from,occ);
    case BISHOP_B:
    case BISHOP_W:
      return get_bishop_attack(from,occ);
    case PROOK_B:
    case PROOK_W:
      return get_prook_attack(from,occ);
    case PBISHOP_B:
    case PBISHOP_W:
      return get_pbishop_attack(from,occ);
    case LANCE_B:
      return get_lance_attack(BLACK,from,occ);
    case LANCE_W:
      return get_lance_attack(WHITE,from,occ);
    default:
      ASSERT(false);
      return Bitboard(0,0);
  }
}
inline Bitboard Position::attack_from(const Color c, const PieceType p, const Square from)const{

  ASSERT(color_is_ok(c));
  ASSERT(square_is_ok(from));
  ASSERT(piece_type_is_ok(p));
  switch(p){
    case PAWN:
      return get_pawn_attack(c,from);
    case KNIGHT:
      return get_knight_attack(c,from);
    case SILVER:
      return get_silver_attack(c,from);
    case GOLD:
    case PPAWN:
    case PLANCE:
    case PKNIGHT:
    case PSILVER:
    case GOLDS:
      return get_gold_attack(c,from);
    case KING:
      return get_king_attack(from);
    default:
      std::cout<<"attack from error no occ"<<p<<std::endl;
      ASSERT(false);
      return Bitboard(0,0);
  }
}
inline Bitboard Position::attack_from
  (const Color c, const PieceType p, const Square from, const Bitboard & occ)const{

  ASSERT(color_is_ok(c));
  ASSERT(square_is_ok(from));
  ASSERT(piece_type_is_ok(p));
  switch(p){
    case LANCE:
      return get_lance_attack(c,from,occ);
    case BISHOP:
      return get_bishop_attack(from,occ);
    case ROOK:
      return get_rook_attack(from,occ);
    case PBISHOP:
      return get_pbishop_attack(from,occ);
    case PROOK:
      return get_prook_attack(from,occ);
    default:
      std::cout<<"attack from error occ"<<p<<std::endl;
      ASSERT(false);
      return Bitboard(0,0);
  }
}
inline Bitboard Position::attack_from(const Piece p, const Square from)const{

  ASSERT(square_is_ok(from));
  ASSERT(piece_is_ok(p));
  switch(p){
    case PAWN_B:
      return get_pawn_attack(BLACK,from);
    case PAWN_W:
      return get_pawn_attack(WHITE,from);
    case KNIGHT_B:
      return get_knight_attack(BLACK,from);
    case KNIGHT_W:
      return get_knight_attack(WHITE,from);
    case SILVER_B:
      return get_silver_attack(BLACK,from);
    case SILVER_W:
      return get_silver_attack(WHITE,from);
    case GOLD_B:
    case PPAWN_B:
    case PLANCE_B:
    case PKNIGHT_B:
    case PSILVER_B:
      return get_gold_attack(BLACK,from);
    case GOLD_W:
    case PPAWN_W:
    case PLANCE_W:
    case PKNIGHT_W:
    case PSILVER_W:
      return get_gold_attack(WHITE,from);
    case KING_B:
    case KING_W:
      return get_king_attack(from);
    case ROOK_B:
    case ROOK_W:
      return get_rook_attack(from,occ_bb());
    case BISHOP_B:
    case BISHOP_W:
      return get_bishop_attack(from,occ_bb());
    case PROOK_B:
    case PROOK_W:
      return get_prook_attack(from,occ_bb());
    case PBISHOP_B:
    case PBISHOP_W:
      return get_pbishop_attack(from,occ_bb());
    case LANCE_B:
      return get_lance_attack(BLACK,from,occ_bb());
    case LANCE_W:
      return get_lance_attack(WHITE,from,occ_bb());
    default:
      ASSERT(false);
      return Bitboard(0,0);
  }
}
inline Bitboard Position::attack_from(const Piece p, const Square from, const Bitboard & occ)const{

  ASSERT(square_is_ok(from));
  ASSERT(piece_is_ok(p));
  switch(p){
    case PAWN_B:
      return get_pawn_attack(BLACK,from);
    case PAWN_W:
      return get_pawn_attack(WHITE,from);
    case KNIGHT_B:
      return get_knight_attack(BLACK,from);
    case KNIGHT_W:
      return get_knight_attack(WHITE,from);
    case SILVER_B:
      return get_silver_attack(BLACK,from);
    case SILVER_W:
      return get_silver_attack(WHITE,from);
    case GOLD_B:
    case PPAWN_B:
    case PLANCE_B:
    case PKNIGHT_B:
    case PSILVER_B:
      return get_gold_attack(BLACK,from);
    case GOLD_W:
    case PPAWN_W:
    case PLANCE_W:
    case PKNIGHT_W:
    case PSILVER_W:
      return get_gold_attack(WHITE,from);
    case KING_B:
    case KING_W:
      return get_king_attack(from);
    case ROOK_B:
    case ROOK_W:
      return get_rook_attack(from,occ);
    case BISHOP_B:
    case BISHOP_W:
      return get_bishop_attack(from,occ);
    case PROOK_B:
    case PROOK_W:
      return get_prook_attack(from,occ);
    case PBISHOP_B:
    case PBISHOP_W:
      return get_pbishop_attack(from,occ);
    case LANCE_B:
      return get_lance_attack(BLACK,from,occ);
    case LANCE_W:
      return get_lance_attack(WHITE,from,occ);
    default:
      ASSERT(false);
      return Bitboard(0,0);
  }
}
inline Bitboard Position::attack_to(const Color c, const Square sq)const{

  ASSERT(square_is_ok(sq));
  ASSERT(color_is_ok(c));
  const Bitboard occ = occ_bb();
  const Color opp = opp_color(c);
  return( (get_pawn_attack(opp,sq) & piece_bb(c,PAWN))
        | (get_lance_attack(opp,sq,occ) & piece_bb(c,LANCE))
        | (get_knight_attack(opp,sq) & piece_bb(c,KNIGHT))
        | (get_silver_attack(opp,sq) & piece_bb(c,SILVER ))
        | (get_gold_attack(opp,sq) & golds_bb(c))
        | (get_king_attack(sq) & (piece_bb(c,PROOK) | piece_bb(c,PBISHOP) | piece_bb(c,KING)))
        | (get_bishop_attack(sq,occ) & (piece_bb(c,BISHOP) | piece_bb(c,PBISHOP)))
        | (get_rook_attack(sq,occ) & (piece_bb(c,ROOK) | piece_bb(c,PROOK))));
}
inline Bitboard Position::attack_to(const Square sq)const{
  return attack_to(sq,occ_bb());
}
inline Bitboard Position::attack_to
  (const Square sq, const Bitboard & occ)const{

 ASSERT(square_is_ok(sq));
  Bitboard ret = (
       (get_pawn_attack(BLACK,sq)   & piece_bb(WHITE,PAWN))
      |(get_pawn_attack(WHITE,sq)   & piece_bb(BLACK,PAWN))
      |(get_knight_attack(BLACK,sq) & piece_bb(WHITE,KNIGHT))
      |(get_knight_attack(WHITE,sq) & piece_bb(BLACK,KNIGHT))
      |(get_silver_attack(BLACK,sq) & piece_bb(WHITE,SILVER))
      |(get_silver_attack(WHITE,sq) & piece_bb(BLACK,SILVER))
      |(get_gold_attack(BLACK,sq)   & golds_bb(WHITE))
      |(get_gold_attack(WHITE,sq)   & golds_bb(BLACK))
      |(get_king_attack(sq)         & piece_bb(KING,PROOK,PBISHOP))
      |(get_bishop_attack(sq,occ)       & piece_bb(BISHOP,PBISHOP))
      );
  Bitboard rook_attack = get_rook_attack(sq,occ);
  ret |= (rook_attack & piece_bb(ROOK,PROOK));
  ret |= (rook_attack & get_pseudo_attack<LANCE>(BLACK,sq) & piece_bb(WHITE,LANCE));
  ret |= (rook_attack & get_pseudo_attack<LANCE>(WHITE,sq) & piece_bb(BLACK,LANCE));
  return ret;
}
extern void init_key();
extern void init_mate_trans();
constexpr int array_to_bb_index[SQUARE_SIZE] ={
  72,63,54,45,36,27,18, 9, 0,
  73,64,55,46,37,28,19,10, 1,
  74,65,56,47,38,29,20,11, 2,
  75,66,57,48,39,30,21,12, 3,
  76,67,58,49,40,31,22,13, 4,
  77,68,59,50,41,32,23,14, 5,
  78,69,60,51,42,33,24,15, 6,
  79,70,61,52,43,34,25,16, 7,
  80,71,62,53,44,35,26,17, 8,
};
constexpr int bb_to_array_index[SQUARE_SIZE] ={
  8,17,26,35,44,53,62,71,80,
  7,16,25,34,43,52,61,70,79,
  6,15,24,33,42,51,60,69,78,
  5,14,23,32,41,50,59,68,77,
  4,13,22,31,40,49,58,67,76,
  3,12,21,30,39,48,57,66,75,
  2,11,20,29,38,47,56,65,74,
  1,10,19,28,37,46,55,64,73,
  0, 9,18,27,36,45,54,63,72,
};
static inline Square array_to_bb(const int xy){
  return (Square)array_to_bb_index[xy];
}
static inline Square bb_to_array(const int xy){
  ASSERT(square_is_ok((Square)xy));
  return (Square) bb_to_array_index[xy];
}


template<int pt>inline Bitboard Position::attack_all(const Color c)const{

  Bitboard piece;
  if(pt == int(GOLD)){
    piece = golds_bb(c);
  }else{
    piece = piece_bb(c,PieceType(pt));
  }
  Bitboard ret(0,0);
  while(piece.is_not_zero()){
    auto from = piece.lsb<D>();
    ret |= attack_from<PieceType(pt)>(c,from);
  }
  constexpr int next = pt - 1;
  ret |= attack_all<next>(c);
  return ret;
}
template<> inline Bitboard Position::attack_all<0>(const Color)const{
    return Bitboard(0,0);
}

#endif
