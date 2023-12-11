#ifndef MOVE_H
#define MOVE_H

#include "type.h"
#include "hand.h"
#include <sstream>
#include <string>

class Position;

//move構成
// prom from to
// 1bit 7bit 7bit

class Move{

  public:
    Move(){}
    explicit Move(u32 m) : value_(m){}
    Move & operator = (const Move & m){
      value_ = m.value_;
      return *this;
    }
    Move & operator = (const volatile Move & m){
      value_ = m.value_;
      return *this;
    }
    void operator = (const Move & m)volatile{
      value_ = m.value_;
    }
    bool operator == (const Move & m)const{
      return value_ == m.value_;
    }
    bool operator != (const Move & m)const{
      return !(*this == m);
    }
    Move(const Square from, const Square to){
      ASSERT(square_is_ok(from));
      ASSERT(square_is_ok(to));
      set(from,to);
    }
    Move(const Square from, const Square to, const bool prom){
      ASSERT(square_is_ok(from));
      ASSERT(square_is_ok(to));
      set(from,to,prom);
    }
    void set(const Square from, const Square to){
      value_ = ((from << shift_) | (to));
    }
    void set(const Square from, const Square to, bool prom){
      value_ = ((from << shift_) | (to));
      if(prom){
        value_ |= prom_flag_;
      }
    }
    void set(const Square to, const HandPiece hp){
      const int f = static_cast<int>(hp) + static_cast<int>(SQUARE_SIZE);
      value_ = ((f << shift_) | (to));
    }
    void set(const u32 m){
      value_ = m;
    }
    void set(const std::string s);
    bool set_sfen(const std::string s);
    u32 value()const{
      return value_;
    }
    inline Square from()const{
      return (Square)((value_>>shift_) & mask_);
    }
    inline Square to()const{
      return (Square)(value_ & mask_);
    }
    inline bool is_prom()const{
      return (value_ & prom_flag_)!=0;
    }
    inline bool is_drop()const{
      return (this->from() >= (SQUARE_SIZE));
    }
    inline HandPiece hand_piece()const{
      ASSERT(is_drop());
      return static_cast<HandPiece>(static_cast<int>(from()) - static_cast<int>(SQUARE_SIZE));
    }
    void print() const {
      std::cout<<str()<<std::endl;
    }
    std::string str(const Position & pos)const;
    std::string str()const;
    std::string str_sfen()const;
    std::string str_csa(const Position & pos)const;
    bool is_ok()const;
    bool is_ok(const Position & pos)const;
    bool is_ok_bonanza(const Position & pos)const;
    static constexpr u32 mask_ = 0x7f;
    static constexpr int shift_ = 7;
    static constexpr int prom_shift_ = 14;
    static constexpr u32 prom_flag_ = 1<<prom_shift_;

  private:

    u32 value_;
};
static inline Move make_move(const Square from, const Square to){
  return static_cast<Move>((from << Move::shift_) | to);
}
static inline Move make_prom_move(const Square from, const Square to){
  return static_cast<Move>((from << Move::shift_) | to | Move::prom_flag_);
}
static inline Move make_drop_move(const Square to, const HandPiece piece){
  ASSERT(hand_piece_is_ok(piece));
  ASSERT(square_is_ok(to));
  return static_cast<Move>((((int)SQUARE_SIZE + (int)piece)<<Move::shift_) | to );
}
static inline Move move_none(){
  return Move(0);
}
static inline Move move_pass(){
  return make_move(SQUARE_SIZE,SQUARE_SIZE);
}
extern Move csa_to_move(const std::string s, const Position & pos);
#endif
