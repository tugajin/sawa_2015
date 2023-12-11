#ifndef PIECE_H
#define PIECE_H

#include "misc.h"
#include "type.h"
#include <iostream>
#include <string>
#include <boost/static_assert.hpp>


enum PieceType{ PROMOTE_PT = 8, EMPTY_T = 0,
  PPAWN, PLANCE, PKNIGHT, PSILVER, PBISHOP, PROOK, KING, GOLD,
  PAWN,  LANCE,  KNIGHT,  SILVER,  BISHOP,  ROOK, PIECE_TYPE_SIZE,
  GOLDS = PIECE_TYPE_SIZE,OCCUPIED = EMPTY_T,
};
enum Piece{
  EMPTY = 0,
  PPAWN_B, PLANCE_B, PKNIGHT_B, PSILVER_B, PBISHOP_B, PROOK_B, KING_B, GOLD_B,
  PAWN_B,  LANCE_B,  KNIGHT_B,  SILVER_B,  BISHOP_B,  ROOK_B, DUMMY_B,
  WHITE_FLAG,
  PPAWN_W, PLANCE_W, PKNIGHT_W, PSILVER_W, PBISHOP_W, PROOK_W, KING_W, GOLD_W,
  PAWN_W,  LANCE_W,  KNIGHT_W,  SILVER_W,  BISHOP_W,  ROOK_W, DUMMY_W,
  PIECE_SIZE, PIECE_SIZE_EX,PROMOTE_P = 8,
};
BOOST_STATIC_ASSERT(DUMMY_B == Piece(15));
constexpr int WHITE_FLAG_SHIFT = 4;
constexpr int PIECE_SIZE_SHIFT = 5;
BOOST_STATIC_ASSERT(static_cast<Piece>(1<<WHITE_FLAG_SHIFT) == WHITE_FLAG);
BOOST_STATIC_ASSERT(PIECE_SIZE == Piece(1<<PIECE_SIZE_SHIFT));

BOOST_STATIC_ASSERT ((int)PPAWN_B    == (int)PPAWN);
BOOST_STATIC_ASSERT ((int)PLANCE_B   == (int)PLANCE);
BOOST_STATIC_ASSERT ((int)PKNIGHT_B  == (int)PKNIGHT);
BOOST_STATIC_ASSERT ((int)PSILVER_B  == (int)PSILVER);
BOOST_STATIC_ASSERT ((int)PBISHOP_B  == (int)PBISHOP);
BOOST_STATIC_ASSERT ((int)PROOK_B    == (int)PROOK);
BOOST_STATIC_ASSERT ((int)KING_B     == (int)KING);
BOOST_STATIC_ASSERT ((int)GOLD_B     == (int)GOLD);
BOOST_STATIC_ASSERT ((int)PAWN_B     == (int)PAWN);
BOOST_STATIC_ASSERT ((int)LANCE_B    == (int)LANCE);
BOOST_STATIC_ASSERT ((int)KNIGHT_B   == (int)KNIGHT);
BOOST_STATIC_ASSERT ((int)SILVER_B   == (int)SILVER);
BOOST_STATIC_ASSERT ((int)BISHOP_B   == (int)BISHOP);
BOOST_STATIC_ASSERT ((int)ROOK_B     == (int)ROOK);

BOOST_STATIC_ASSERT((1<<WHITE_FLAG_SHIFT) == WHITE_FLAG);
BOOST_STATIC_ASSERT(EMPTY == 0);
BOOST_STATIC_ASSERT(WHITE_FLAG == 16);
BOOST_STATIC_ASSERT(DUMMY_B == 15);

enum_op(PieceType);
enum_op(Piece);

const std::string piece_name[] ={
  "・",
  "と","杏","圭","全","馬","龍","玉","金","歩","香","桂","銀","角","飛",
  "？","？",
  "と","杏","圭","全","馬","龍","玉","金","歩","香","桂","銀","角","飛",
  "？","？",
};
static inline bool piece_type_is_ok(const PieceType p){

  if(p < PPAWN){
    return false;
  }
  if(p > PIECE_TYPE_SIZE){
    return false;
  }
  return true;
}
static inline bool piece_is_ok(const Piece piece){

  if(piece < PPAWN_B){
    std::cout<<"piece is too small"<<piece<<std::endl;
    return false;
  }
  if(piece > ROOK_W){
    std::cout<<"piece is too big"<<piece<<std::endl;
    return false;
  }
  return true;
}

static inline Color piece_color(const Piece p){
  ASSERT(piece_is_ok(p));
  return (p > ROOK_B) ? (WHITE) : (BLACK);
}
static inline PieceType piece_to_piece_type(const Piece p){
  ASSERT(p == EMPTY || piece_is_ok(p));
  return (PieceType)(p & DUMMY_B);
}
template<Color c>static inline Piece piece_type_to_piece(const PieceType p){
  ASSERT(piece_type_is_ok(p));
  return static_cast<Piece>(static_cast<int>(p) | (static_cast<int>(c)<<WHITE_FLAG_SHIFT));
}
static inline Piece piece_type_to_piece(PieceType p, Color c){
  ASSERT(piece_type_is_ok(p));
  ASSERT(color_is_ok(c));
  return (c == BLACK) ? piece_type_to_piece<BLACK>(p) : piece_type_to_piece<WHITE>(p);
}
static inline bool piece_is_slider(const Piece p){
  ASSERT(piece_is_ok(p));
  return  ( (p == LANCE_B || p == BISHOP_B || p == ROOK_B || p == PBISHOP_B || p == PROOK_B)
          ||(p == LANCE_W || p == BISHOP_W || p == ROOK_W || p == PBISHOP_W || p == PROOK_W));
}
static inline bool piece_type_is_slider(const PieceType p){
  ASSERT(piece_type_is_ok(p));
  return (p == LANCE || p == BISHOP || p == ROOK || p == PBISHOP || p == PROOK);
}
static inline bool can_prom_piece(const Piece p){
  ASSERT(piece_is_ok(p));
  return ((p >= PAWN_B && p <= ROOK_B) | (p >= PAWN_W && p <= ROOK_W));
}
static inline bool can_prom_piece_type(const PieceType pt){
  ASSERT(piece_type_is_ok(pt));
  return (pt >= PAWN && pt <= ROOK);
}
static inline bool promed_piece_type(const PieceType pt){
  ASSERT(piece_type_is_ok(pt));
  return (pt <= PROOK);
}
static inline bool promed_piece(const Piece pt){
  ASSERT(piece_is_ok(pt));
  return (piece_to_piece_type(pt) <= PROOK);
}
static inline Piece prom_piece(const Piece p){
  ASSERT(piece_is_ok(p));
  return static_cast<Piece>(static_cast<int>(p) - static_cast<int>(PROMOTE_P));
}
static inline PieceType prom_piece_type(const PieceType pt){
  ASSERT(piece_type_is_ok(pt));
  return static_cast<PieceType>(static_cast<int>(pt) - static_cast<int>(PROMOTE_PT));
}
static inline Piece unprom_piece(const Piece p){
  ASSERT(piece_is_ok(p));
  return static_cast<Piece>(static_cast<int>(p) | static_cast<int>(PROMOTE_P));
}
static inline PieceType unprom_piece_type(const PieceType pt){
  ASSERT(piece_type_is_ok(pt));
  return static_cast<PieceType>(static_cast<int>(pt) | static_cast<int>(PROMOTE_PT));
}
static inline bool piece_type_is_promed(const PieceType pt){
  ASSERT(pt == EMPTY_T || piece_type_is_ok(pt));
  return (pt >= PPAWN && pt <= PROOK);
}
static inline Piece opp_piece(const Piece p){
  ASSERT(piece_is_ok(p));
  return static_cast<Piece>(p ^ WHITE_FLAG);
}
extern std::string str_piece_name(const Piece p);
extern std::string str_csa_piece_name(const PieceType pt);
extern std::string str_piece_type_name(const PieceType p);
extern PieceType csa_piece_to_piece_type(const std::string s);
extern Piece sfen_to_piece(const std::string s);
extern std::string piece_to_sfen(const Piece p);

#endif
