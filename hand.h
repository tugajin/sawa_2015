#ifndef HAND_H
#define HAND_H

#include "type.h"
#include "misc.h"
#include "piece.h"
#include <string>

enum HandPiece{//if change it, change hand_inc,hand_shift,hand_mask,
  GOLD_H, PAWN_H, LANCE_H, KNIGHT_H, SILVER_H, BISHOP_H, ROOK_H,
  HAND_PIECE_SIZE, HAND_NULL_H = HAND_PIECE_SIZE,
};

enum HandState { HAND_IS_WIN, HAND_IS_LOSE, HAND_IS_SAME, HAND_IS_UNKNOWN, };

enum_op(HandPiece);
#define HAND_FOREACH(hp) for(HandPiece hp = GOLD_H; hp < HAND_PIECE_SIZE; ++hp)
//handの構成
// rook bishop gold silver knight lance pawn
// 2bit 2bit   3bit  3bit  3bit   3bit   5bit
const std::string hand_piece_name[HAND_PIECE_SIZE] ={
  "金","歩","香","桂","銀","角","飛",
};
class Hand{
  public:
    Hand(){}
    Hand(const u32 h){
      value_ = h;
    }
    Hand(const HandPiece p, const int num){
      ASSERT(num >= 0);
      ASSERT(num <= 18);
      set(p,num);
    }
    void set(const u32 h){
      value_ = h;
    }
    void set(const HandPiece p, const int num){
      value_ = hand_inc[p] * num;
    }
    void set(const HandPiece p, const int num,
             const HandPiece p2, const int num2){
      value_ = hand_inc[p] * num + hand_inc[p2] * num2;
    }
    void set(const HandPiece p , const int num,
             const HandPiece p2, const int num2,
             const HandPiece p3, const int num3){
      value_ = hand_inc[p] * num
             + hand_inc[p2] * num2
             + hand_inc[p3] * num3;
    }
    void set(const HandPiece p , const int num,
             const HandPiece p2, const int num2,
             const HandPiece p3, const int num3,
             const HandPiece p4, const int num4){
      value_ = hand_inc[p] * num
             + hand_inc[p2] * num2
             + hand_inc[p3] * num3
             + hand_inc[p4] * num4
             ;
    }
    void set(const HandPiece p , const int num,
             const HandPiece p2, const int num2,
             const HandPiece p3, const int num3,
             const HandPiece p4, const int num4,
             const HandPiece p5, const int num5){
      value_ = hand_inc[p] * num
             + hand_inc[p2] * num2
             + hand_inc[p3] * num3
             + hand_inc[p4] * num4
             + hand_inc[p5] * num5
             ;
    }
    void set(const HandPiece p , const int num,
             const HandPiece p2, const int num2,
             const HandPiece p3, const int num3,
             const HandPiece p4, const int num4,
             const HandPiece p5, const int num5,
             const HandPiece p6, const int num6){
      value_ = hand_inc[p] * num
             + hand_inc[p2] * num2
             + hand_inc[p3] * num3
             + hand_inc[p4] * num4
             + hand_inc[p5] * num5
             + hand_inc[p6] * num6
             ;
    }
    void set(const HandPiece p , const int num,
             const HandPiece p2, const int num2,
             const HandPiece p3, const int num3,
             const HandPiece p4, const int num4,
             const HandPiece p5, const int num5,
             const HandPiece p6, const int num6,
             const HandPiece p7, const int num7){
      value_ =  hand_inc[p] * num
              + hand_inc[p2] * num2
              + hand_inc[p3] * num3
              + hand_inc[p4] * num4
              + hand_inc[p5] * num5
              + hand_inc[p6] * num6
              + hand_inc[p7] * num7
              ;
    }
    void set_all(const int num[]){
      value_ = hand_inc[GOLD_H] * num[GOLD_H]
             + hand_inc[PAWN_H] * num[PAWN_H]
             + hand_inc[LANCE_H] * num[LANCE_H]
             + hand_inc[KNIGHT_H] * num[KNIGHT_H]
             + hand_inc[SILVER_H] * num[SILVER_H]
             + hand_inc[BISHOP_H] * num[BISHOP_H]
             + hand_inc[ROOK_H] * num[ROOK_H];
    }
    u32 value()const{
      return value_;
    }
    inline int num(const HandPiece p)const{
      return (u32)((value_ & hand_mask[p]) >> hand_shift[p]);
    }
    inline int num()const{
      int ret = 0;
      HAND_FOREACH(hp){
        ret += num(hp);
      }
      return ret;
    }
    inline bool is_have(const HandPiece p)const{
      return (value_ & hand_mask[p])!=0;
    }
    inline void inc(const HandPiece p){
      value_+=hand_inc[p];
    }
    inline void dec(const HandPiece p){
      value_-=hand_inc[p];
    }
    inline bool is_not_zero()const{
      return value_!=0;
    }
    bool operator == (const Hand & rhs) const {
      return rhs.value() == this->value();
    }
    bool operator != (const Hand & rhs) const {
      return rhs.value() != this->value();
    }
    bool operator >= (const Hand & rhs) const{
      return ( this->num(GOLD_H)    >= rhs.num(GOLD_H)
          && this->num(PAWN_H)    >= rhs.num(PAWN_H)
          && this->num(LANCE_H)  >= rhs.num(LANCE_H)
          && this->num(KNIGHT_H) >= rhs.num(KNIGHT_H)
          && this->num(SILVER_H) >= rhs.num(SILVER_H)
          && this->num(GOLD_H)   >= rhs.num(GOLD_H)
          && this->num(BISHOP_H) >= rhs.num(BISHOP_H)
          && this->num(ROOK_H)   >= rhs.num(ROOK_H));
    }
    bool operator <= (const Hand & rhs) const{
      return ( this->num(GOLD_H)    <= rhs.num(GOLD_H)
          && this->num(PAWN_H)    <= rhs.num(PAWN_H)
          && this->num(LANCE_H)  <= rhs.num(LANCE_H)
          && this->num(KNIGHT_H) <= rhs.num(KNIGHT_H)
          && this->num(SILVER_H) <= rhs.num(SILVER_H)
          && this->num(GOLD_H)   <= rhs.num(GOLD_H)
          && this->num(BISHOP_H) <= rhs.num(BISHOP_H)
          && this->num(ROOK_H)   <= rhs.num(ROOK_H));
    }
    bool operator > (const Hand & rhs) const{
      if(*(this) == rhs){
        return false;
      }
      return (*(this) >= rhs);
    }
    bool operator < (const Hand & rhs) const{
      if(*(this) == rhs){
        return false;
      }
      return (*(this) <= rhs);
    }

  private:

    u32 value_;

    static const u32 hand_inc[HAND_PIECE_SIZE] ;
    static const u32 hand_shift[HAND_PIECE_SIZE];
    static const u32 hand_mask[HAND_PIECE_SIZE];

};
inline bool hand_piece_is_ok(const HandPiece hp){

  if(hp < GOLD_H){
    std::cout<<"hand piece error "<<hp<<std::endl;
    return false;
  }else if(hp > ROOK_H){
    std::cout<<"hand piece error "<<hp<<std::endl;
    return false;
  }
  return true;
}
static inline HandPiece piece_type_to_hand_piece(const PieceType pt){
  ASSERT(piece_type_is_ok(pt));
  const auto unpt = unprom_piece_type(pt);
  return static_cast<HandPiece>(static_cast<int>(unpt) - static_cast<int>(GOLD));
}
static inline HandPiece piece_to_hand_piece(const Piece p){
  ASSERT(piece_is_ok(p));
  return piece_type_to_hand_piece(piece_to_piece_type(p));
}
static inline PieceType hand_piece_to_piece_type(const HandPiece hp){
  ASSERT(hand_piece_is_ok(hp));
  return static_cast<PieceType>(static_cast<int>(hp) + static_cast<int>(GOLD));
}
static inline Piece hand_piece_to_piece(const HandPiece hp, const Color c){
  ASSERT(hand_piece_is_ok(hp));
  ASSERT(color_is_ok(c));
  return static_cast<Piece>((hand_piece_to_piece_type(hp)) | (c << WHITE_FLAG_SHIFT));
}
static inline std::string hand_piece_str(const HandPiece p){
    ASSERT(hand_piece_is_ok(p));
    return hand_piece_name[p];
}
static inline HandState hand_cmp(const Hand & h1, const Hand & h2){

  if(h1 > h2){
    return HAND_IS_WIN;
  }else if(h1 < h2){
    return HAND_IS_LOSE;
  }else if(h1 == h2){
    return HAND_IS_SAME;
  }
  return HAND_IS_UNKNOWN;
}


#endif
