#ifndef CHECK_INFO
#define CHECK_INFO

#include "type.h"
#include "bitboard.h"

class Position;

class CheckInfo{
  public:
    CheckInfo(){};
    CheckInfo(const Position & pos);
    inline Bitboard pinned()const{
      return pinned_;
    }
    inline Bitboard check_sq(const PieceType pt)const{
      ASSERT(piece_type_is_ok(pt));
      return check_sq_[pt];
    }
    inline Bitboard discover_checker()const{
      return discover_checker_;
    }
    inline Square king_sq()const{
      return king_sq_;
    }
    void dump()const;
    bool is_ok()const;
  private:
    Bitboard pinned_;
    Bitboard check_sq_[PIECE_TYPE_SIZE];
    Bitboard discover_checker_;
    Square king_sq_;
};
#endif
