#ifndef TYPE_H
#define TYPE_H

#include <iostream>
#include <cassert>
#include "misc.h"
#include <boost/static_assert.hpp>
#include <boost/current_function.hpp>

typedef unsigned long long int u64;
typedef long long int s64;
typedef unsigned int u32;
typedef short s16;
typedef char s8;
typedef unsigned char u8;
typedef unsigned short u16;

typedef unsigned long long int Key;
typedef short Kvalue;

enum Depth { DEPTH_NONE = 0, DEPTH_ONE = 2, DEPTH_QS = -1 };
enum Value { VALUE_MATE = 32600, VALUE_INF = 32768,
             VALUE_NONE = -VALUE_MATE, VALUE_EVAL_MAX = 32000,
             VALUE_ZERO = 0, VALUE_REP = -30000/*299*/,
             VALUE_REP_WIN = 32601, STORED_EVAL_IS_EMPTY = -9999999 };
enum Color { BLACK = 0, WHITE = 1, COLOR_SIZE,
             COLOR_ILLEGAL = COLOR_SIZE };
enum Square {
  A1 = 0,
  SQUARE_SIZE = 81,
  LEFT = 9,RIGHT = -9, UP = -1, DOWN = 1,
  LEFT_UP = LEFT + UP,
  LEFT_DOWN = LEFT + DOWN,
  RIGHT_UP = RIGHT + UP,
  RIGHT_DOWN = RIGHT + DOWN,
  WALL = -1,
};
enum File { FILE_A = 0, FILE_B, FILE_C,
            FILE_D, FILE_E, FILE_F,
            FILE_G, FILE_H, FILE_I,
            FILE_SIZE, FILE_LEFT = 1, FILE_RIGHT = -1 };
enum Rank { RANK_1 = 0, RANK_2, RANK_3,
            RANK_4, RANK_5 ,RANK_6,
            RANK_7, RANK_8, RANK_9,
            RANK_SIZE, RANK_UP = -1, RANK_DOWN = 1 };
enum SquareRelation{ NONE_RELATION,
                    RANK_RELATION, FILE_RELATION,
                    LEFT_UP_RELATION,RIGHT_UP_RELATION, };

enum NodeType { PV_NODE, NOPV_NODE, };

BOOST_STATIC_ASSERT(RANK_1 == 0);
BOOST_STATIC_ASSERT(FILE_A == 0);
BOOST_STATIC_ASSERT(FILE_SIZE == 9);
BOOST_STATIC_ASSERT(RANK_SIZE == 9);
BOOST_STATIC_ASSERT(NONE_RELATION == 0);

constexpr int MAX_PLY = 128;
constexpr int MAX_MOVE_NUM = 600;
enum_op(Color);
enum_op(Square);
enum_op(File);
enum_op(Rank);
enum_op(Value);
enum_op(Depth);

//N -> only check bit
//F -> check bit and delete

enum BitOp { N, D };

constexpr Rank square_rank[SQUARE_SIZE]={
  RANK_1,RANK_2,RANK_3,RANK_4,RANK_5,RANK_6,RANK_7,RANK_8,RANK_9,
  RANK_1,RANK_2,RANK_3,RANK_4,RANK_5,RANK_6,RANK_7,RANK_8,RANK_9,
  RANK_1,RANK_2,RANK_3,RANK_4,RANK_5,RANK_6,RANK_7,RANK_8,RANK_9,
  RANK_1,RANK_2,RANK_3,RANK_4,RANK_5,RANK_6,RANK_7,RANK_8,RANK_9,
  RANK_1,RANK_2,RANK_3,RANK_4,RANK_5,RANK_6,RANK_7,RANK_8,RANK_9,
  RANK_1,RANK_2,RANK_3,RANK_4,RANK_5,RANK_6,RANK_7,RANK_8,RANK_9,
  RANK_1,RANK_2,RANK_3,RANK_4,RANK_5,RANK_6,RANK_7,RANK_8,RANK_9,
  RANK_1,RANK_2,RANK_3,RANK_4,RANK_5,RANK_6,RANK_7,RANK_8,RANK_9,
  RANK_1,RANK_2,RANK_3,RANK_4,RANK_5,RANK_6,RANK_7,RANK_8,RANK_9,
};
constexpr File square_file[SQUARE_SIZE]={
  FILE_A,FILE_A,FILE_A,FILE_A,FILE_A,FILE_A,FILE_A,FILE_A,FILE_A,
  FILE_B,FILE_B,FILE_B,FILE_B,FILE_B,FILE_B,FILE_B,FILE_B,FILE_B,
  FILE_C,FILE_C,FILE_C,FILE_C,FILE_C,FILE_C,FILE_C,FILE_C,FILE_C,
  FILE_D,FILE_D,FILE_D,FILE_D,FILE_D,FILE_D,FILE_D,FILE_D,FILE_D,
  FILE_E,FILE_E,FILE_E,FILE_E,FILE_E,FILE_E,FILE_E,FILE_E,FILE_E,
  FILE_F,FILE_F,FILE_F,FILE_F,FILE_F,FILE_F,FILE_F,FILE_F,FILE_F,
  FILE_G,FILE_G,FILE_G,FILE_G,FILE_G,FILE_G,FILE_G,FILE_G,FILE_G,
  FILE_H,FILE_H,FILE_H,FILE_H,FILE_H,FILE_H,FILE_H,FILE_H,FILE_H,
  FILE_I,FILE_I,FILE_I,FILE_I,FILE_I,FILE_I,FILE_I,FILE_I,FILE_I,
};

const std::string sfen_file[FILE_SIZE] ={
"1","2","3","4","5","6","7","8","9",
};
const std::string sfen_rank[RANK_SIZE] = {
"a","b","c","d","e","f","g","h","i",
};

extern SquareRelation square_relation[SQUARE_SIZE][SQUARE_SIZE];
extern void init_relation();
extern bool value_is_ok(const Value v);

#define CACHE_LINE_SIZE 64
#if defined(__INTEL_COMPILER)
#define CACHE_LINE_ALIGNMENT __declspec(align(CACHE_LINE_SIZE))
#else
#define CACHE_LINE_ALIGNMENT __attribute__((aligned(CACHE_LINE_ALIGNMENT)))
#endif

#define FORCE_INLINE  __attribute__((always_inline))

#ifndef NDEBUG
#if 1
#define ASSERT(c) do{\
                    if(!(c)){\
                      printf("file : %s line : %d condition : %s\n",__FILE__, __LINE__, #c);\
                      exit(1);\
                    }\
                    }while(false)
#else
#define ASSERT(c) assert(c)
#endif
#else
#define ASSERT(c)
#endif
#define FILE_FOREACH(f) for(File (f) = FILE_A; (f) < FILE_SIZE; ++(f))
#define RANK_FOREACH(r) for(Rank (r) = RANK_1; (r) < RANK_SIZE; ++(r))
#define SQUARE_FOREACH(sq) for(Square (sq) = A1; (sq) < SQUARE_SIZE; ++(sq))
#define COLOR_FOREACH(col) for(Color (col) = BLACK; (col) < COLOR_SIZE; ++(col))
#define PIECE_TYPE_FOREACH(pt) for(PieceType (pt) = PPAWN; (pt) < PIECE_TYPE_SIZE; (++pt))
#define B_PIECE_FOREACH(p) for(Piece (p) = PPAWN_B; (p) <= ROOK_B; (++p))
#define W_PIECE_FOREACH(p) for(Piece (p) = PPAWN_W; (p) <= ROOK_W; (++p))

inline bool color_is_ok(const Color c){

  if(c == BLACK || c == WHITE){
    return true;
  }else{
    return false;
  }
}
inline bool square_is_ok(const Square xy){

  if(xy < A1){ return false;  }
  if(xy >= SQUARE_SIZE) { return false; }
  return true;
}
inline bool rank_is_ok(const Rank r){

 if(r < RANK_1){
    return false;
  }
  if(r > RANK_9){
    return false;
  }
  return true;
}
inline bool file_is_ok(const File f){

  if(f < FILE_A){
    return false;
  }
  if(f > FILE_I){
    return false;
  }
  return true;
}
static inline constexpr Color opp_color(const Color c){
  //BOOST_STATIC_ASSERT(color_is_ok(c));
  return static_cast<Color>(static_cast<int>(c)^1);
}
static inline File square_to_file(const Square s){
  ASSERT(square_is_ok(s));
  return square_file[s];
}
static inline Rank square_to_rank(const Square s){
  ASSERT(square_is_ok(s));
  return square_rank[s];
}
static inline Square make_square(const File f, const Rank r){
  ASSERT(file_is_ok(f));
  ASSERT(rank_is_ok(r));
  return static_cast<Square>(
    (static_cast<int>(f) * static_cast<int>(FILE_SIZE)) + static_cast<int>(r)) ;
}
static inline Square make_square(const int ff, const int rr){
  const auto f = static_cast<File>(ff - 1);
  const auto r = static_cast<Rank>(rr - 1);
  ASSERT(file_is_ok(f));
  ASSERT(rank_is_ok(r));
  return static_cast<Square>(
    (static_cast<int>(f) * static_cast<int>(FILE_SIZE)) + static_cast<int>(r)) ;
}
template<Color c,Rank tr> static inline bool is_include_rank(const Rank r){
  ASSERT(color_is_ok(c));
  ASSERT(rank_is_ok(r));
  ASSERT(rank_is_ok(tr));
  return (c == BLACK) ? (r <= tr) : (r >= (RANK_9-tr));
}
template<Rank tr> static inline bool is_include_rank(const Rank r, const Color c){
  ASSERT(color_is_ok(c));
  ASSERT(rank_is_ok(r));
  ASSERT(rank_is_ok(tr));
  return (c == BLACK) ? (r <= tr) : (r >= (RANK_9-tr));
}
template<Color c, Rank tr>static inline bool is_include_rank(const Square sq){
  const Rank r = square_to_rank(sq);
  return is_include_rank<c,tr>(r);
}
template<Color c>static inline bool is_prom_rank(const Rank r){
  ASSERT(color_is_ok(c));
  ASSERT(rank_is_ok(r));
  return (c == BLACK) ? (r <= RANK_3) : (r >= RANK_7);
}
static inline bool is_prom_rank(const Rank r, const Color c){
  return (c == BLACK) ? is_prom_rank<BLACK>(r) : is_prom_rank<WHITE>(r);
}
template<Color c>static inline bool is_prom_square(const Square sq){
  return (c == BLACK) ? is_prom_rank<BLACK>(square_to_rank(sq))
                      : is_prom_rank<WHITE>(square_to_rank(sq));
}
static inline bool is_prom_square(const Square sq, const Color c){
  return (c == BLACK) ? is_prom_rank<BLACK>(square_to_rank(sq))
                      : is_prom_rank<WHITE>(square_to_rank(sq));
}
static inline bool square_is_drop(const Square s){
  return (s >= SQUARE_SIZE);
}
inline bool ply_is_ok(const int p){

  if(p < 0){
    return false;
  }else if(p >= MAX_PLY){
    return false;
  }
  return true;
}
static inline SquareRelation get_square_relation(const Square sq1, const Square sq2){
  ASSERT(square_is_ok(sq1));
  ASSERT(square_is_ok(sq2));
  return square_relation[sq1][sq2];
}
static inline Value value_mate(const int ply){
  return -VALUE_MATE + static_cast<Value>(ply);
}
static inline Square opp_square(const Square sq){
  return static_cast<Square>(80 - static_cast<int>(sq));
}
static inline std::string file_to_sfen_file(const File f){
  ASSERT(file_is_ok(f));
  return sfen_file[f];
}
static inline std::string rank_to_sfen_rank(const Rank r){
  ASSERT(rank_is_ok(r));
  return sfen_rank[r];
}
static inline File sfen_file_to_file(const std::string s){

  FILE_FOREACH(f){
    if(s == sfen_file[f]){
      return f;
    }
  }
  return FILE_SIZE;
}
static inline Rank sfen_rank_to_rank(const std::string s){

  RANK_FOREACH(r){
    if(s == sfen_rank[r]){
      return r;
    }
  }
  return RANK_SIZE;
}
#endif
