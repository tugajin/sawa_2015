#ifndef BITBOARD_H
#define BITBOARD_H

#include "type.h"
#include "piece.h"

#define HAVE_SSE4

#if defined (HAVE_SSE4)
#include <smmintrin.h>
#endif

class Bitboard{
  public:
#if defined(HAVE_SSE4)
    Bitboard & operator = (const Bitboard & rhs){
      _mm_store_si128(&this->m_, rhs.m_);
      return *this;
    }
    Bitboard(const Bitboard & rhs){
      _mm_store_si128(&this->m_, rhs.m_);
    }
#endif
    Bitboard() {}
    Bitboard(const u64 v0, const u64 v1) {
      this->bb_[0] = v0;
      this->bb_[1] = v1;
    }
    u64 bb(const int index) const {
      ASSERT(index == 0 || index == 1);
      return bb_[index];
    }
#if defined(HAVE_SSE4)
    __m128i bb() const{
      return this->m_;
    }
#endif
    void set(const u64 v0, const u64 v1) {
      this->bb_[0] = v0;
      this->bb_[1] = v1;
    }
#if defined(HAVE_SSE4)
    void set(const __m128i v) {
      _mm_store_si128(&this->m_, v);
    }
#endif
    u64 merge() const {
      return this->bb(0) | this->bb(1);
    }
    bool is_not_zero() const {
#if defined(HAVE_SSE4)
      return !(_mm_testz_si128(this->m_, _mm_set1_epi8(static_cast<char>(0xffu))));
#else
      return (this->merge() ? true : false);
#endif
    }
    bool and_is_not_zero(const Bitboard& bb) const {
#if defined(HAVE_SSE4)
      return !(_mm_testz_si128(this->m_, bb.m_));
#else
      return (*this & bb).is_not_zero();
#endif
    }
    Bitboard operator ~ () const {
      Bitboard tmp;
#if defined(HAVE_SSE4)
      _mm_store_si128(&tmp.m_
          , _mm_andnot_si128(this->m_, _mm_set1_epi8(static_cast<char>(0xffu))));
#else
      tmp.bb_[0] = ~this->bb(0);
      tmp.bb_[1] = ~this->bb(1);
#endif
      return tmp;
    }
    Bitboard operator &= (const Bitboard& rhs) {
#if defined(HAVE_SSE4)
      _mm_store_si128(&this->m_, _mm_and_si128(this->m_, rhs.m_));
#else
      this->bb_[0] &= rhs.bb(0);
      this->bb_[1] &= rhs.bb(1);
#endif
      return *this;
    }
    Bitboard operator |= (const Bitboard& rhs) {
#if defined(HAVE_SSE4)
      _mm_store_si128(&this->m_, _mm_or_si128(this->m_, rhs.m_));
#else
      this->bb_[0] |= rhs.bb(0);
      this->bb_[1] |= rhs.bb(1);
#endif
      return *this;
    }
    Bitboard operator ^= (const Bitboard& rhs) {
#if defined(HAVE_SSE4)
      _mm_store_si128(&this->m_, _mm_xor_si128(this->m_, rhs.m_));
#else
      this->bb_[0] ^= rhs.bb(0);
      this->bb_[1] ^= rhs.bb(1);
#endif
      return *this;
    }
    Bitboard operator <<= (const int i) {
#if defined(HAVE_SSE4)
      _mm_store_si128(&this->m_, _mm_slli_epi64(this->m_, i));
#else
      this->bb_[0] <<= i;
      this->bb_[1] <<= i;
#endif
      return *this;
    }
    Bitboard operator >>= (const int i) {
#if defined(HAVE_SSE4)
      _mm_store_si128(&this->m_, _mm_srli_epi64(this->m_, i));
#else
      this->bb_[0] >>= i;
      this->bb_[1] >>= i;
#endif
      return *this;
    }
    Bitboard operator & (const Bitboard& rhs) const {
      return Bitboard(*this) &= rhs;
    }
    Bitboard operator | (const Bitboard& rhs) const {
      return Bitboard(*this) |= rhs;
    }
    Bitboard operator ^ (const Bitboard& rhs) const {
      return Bitboard(*this) ^= rhs;
    }
    Bitboard operator << (const int i) const {
      return Bitboard(*this) <<= i;
    }
    Bitboard operator >> (const int i) const {
      return Bitboard(*this) >>= i;
    }
    bool operator == (const Bitboard& rhs) const {
#if defined(HAVE_SSE4)
      return (_mm_testc_si128(_mm_cmpeq_epi8(this->m_, rhs.m_)
            ,_mm_set1_epi8(static_cast<char>(0xffu))) ? true : false);
#else
      return ((this->bb(0) == rhs.bb(0)) && (this->bb(1) == rhs.bb(1))) ? true : false;
#endif
    }
    bool operator != (const Bitboard& rhs) const {
      return !(*this == rhs);
    }
    bool operator < (const Bitboard& rhs) const {
      if(this->bb(0) != rhs.bb(0)){
        return (this->bb(0) < rhs.bb(0));
      }
      return (this->bb(1) < rhs.bb(1));
    }
    bool operator > (const Bitboard& rhs) const {
      if(this->bb(0) != rhs.bb(0)){
        return (this->bb(0) > rhs.bb(0));
      }
      return (this->bb(1) > rhs.bb(1));
    }
    Bitboard not_this_and(const Bitboard& bb) const {
#if defined(HAVE_SSE4)
      Bitboard tmp;
      _mm_store_si128(&tmp.m_, _mm_andnot_si128(this->m_, bb.m_));
      return tmp;
#else
      return ~(*this) & bb;
#endif
    }
    inline int pop_cnt() const;
    template<BitOp o>inline Square lsb_right();
    template<BitOp o>inline Square lsb_left();
    template<BitOp o>inline Square lsb(){
      ASSERT(is_not_zero());
      if(this->bb(0)){
        return lsb_right<o>();
      }
      return lsb_left<o>();
    }
    template<BitOp o>inline Square msb_right();
    template<BitOp o>inline Square msb_left();
    template<BitOp o>inline Square msb(){
      ASSERT(is_not_zero());
      if(this->bb(0)){
        return msb_right<o>();
      }
      return msb_left<o>();
    }
    bool is_seted(const Square sq)const;
    void And(const Square sq);
    void Or(const Square sq);
    void Xor(const Square sq);
    void print()const;
  private:
#if defined(HAVE_SSE4)
    union{
      u64 bb_[2];
      __m128i m_;
    };
#else
    u64 bb_[2];
#endif
};

const Bitboard mask[SQUARE_SIZE] = {
  Bitboard((1ull)<< 0,0ull),
  Bitboard((1ull)<< 1,0ull),
  Bitboard((1ull)<< 2,0ull),
  Bitboard((1ull)<< 3,0ull),
  Bitboard((1ull)<< 4,0ull),
  Bitboard((1ull)<< 5,0ull),
  Bitboard((1ull)<< 6,0ull),
  Bitboard((1ull)<< 7,0ull),
  Bitboard((1ull)<< 8,0ull),
  Bitboard((1ull)<< 9,0ull),
  Bitboard((1ull)<<10,0ull),
  Bitboard((1ull)<<11,0ull),
  Bitboard((1ull)<<12,0ull),
  Bitboard((1ull)<<13,0ull),
  Bitboard((1ull)<<14,0ull),
  Bitboard((1ull)<<15,0ull),
  Bitboard((1ull)<<16,0ull),
  Bitboard((1ull)<<17,0ull),
  Bitboard((1ull)<<18,0ull),
  Bitboard((1ull)<<19,0ull),
  Bitboard((1ull)<<20,0ull),
  Bitboard((1ull)<<21,0ull),
  Bitboard((1ull)<<22,0ull),
  Bitboard((1ull)<<23,0ull),
  Bitboard((1ull)<<24,0ull),
  Bitboard((1ull)<<25,0ull),
  Bitboard((1ull)<<26,0ull),
  Bitboard((1ull)<<27,0ull),
  Bitboard((1ull)<<28,0ull),
  Bitboard((1ull)<<29,0ull),
  Bitboard((1ull)<<30,0ull),
  Bitboard((1ull)<<31,0ull),
  Bitboard((1ull)<<32,0ull),
  Bitboard((1ull)<<33,0ull),
  Bitboard((1ull)<<34,0ull),
  Bitboard((1ull)<<35,0ull),
  Bitboard((1ull)<<36,0ull),
  Bitboard((1ull)<<37,0ull),
  Bitboard((1ull)<<38,0ull),
  Bitboard((1ull)<<39,0ull),
  Bitboard((1ull)<<40,0ull),
  Bitboard((1ull)<<41,0ull),
  Bitboard((1ull)<<42,0ull),
  Bitboard((1ull)<<43,0ull),
  Bitboard((1ull)<<44,0ull),
  Bitboard((1ull)<<45,0ull),
  Bitboard((1ull)<<46,0ull),
  Bitboard((1ull)<<47,0ull),
  Bitboard((1ull)<<48,0ull),
  Bitboard((1ull)<<49,0ull),
  Bitboard((1ull)<<50,0ull),
  Bitboard((1ull)<<51,0ull),
  Bitboard((1ull)<<52,0ull),
  Bitboard((1ull)<<53,0ull),
  Bitboard((1ull)<<54,0ull),
  Bitboard((1ull)<<55,0ull),
  Bitboard((1ull)<<56,0ull),
  Bitboard((1ull)<<57,0ull),
  Bitboard((1ull)<<58,0ull),
  Bitboard((1ull)<<59,0ull),
  Bitboard((1ull)<<60,0ull),
  Bitboard((1ull)<<61,0ull),
  Bitboard((1ull)<<62,0ull),
  Bitboard(0ull,(1ull)<< 0),
  Bitboard(0ull,(1ull)<< 1),
  Bitboard(0ull,(1ull)<< 2),
  Bitboard(0ull,(1ull)<< 3),
  Bitboard(0ull,(1ull)<< 4),
  Bitboard(0ull,(1ull)<< 5),
  Bitboard(0ull,(1ull)<< 6),
  Bitboard(0ull,(1ull)<< 7),
  Bitboard(0ull,(1ull)<< 8),
  Bitboard(0ull,(1ull)<< 9),
  Bitboard(0ull,(1ull)<<10),
  Bitboard(0ull,(1ull)<<11),
  Bitboard(0ull,(1ull)<<12),
  Bitboard(0ull,(1ull)<<13),
  Bitboard(0ull,(1ull)<<14),
  Bitboard(0ull,(1ull)<<15),
  Bitboard(0ull,(1ull)<<16),
  Bitboard(0ull,(1ull)<<17),
};

const Bitboard prom_area[COLOR_SIZE] = {
  ( mask[72]| mask[63]| mask[54]| mask[45]| mask[36]| mask[27]| mask[18]| mask[ 9]| mask[ 0]|
    mask[73]| mask[64]| mask[55]| mask[46]| mask[37]| mask[28]| mask[19]| mask[10]| mask[ 1]|
    mask[74]| mask[65]| mask[56]| mask[47]| mask[38]| mask[29]| mask[20]| mask[11]| mask[ 2]),
  ( mask[78]| mask[69]| mask[60]| mask[51]| mask[42]| mask[33]| mask[24]| mask[15]| mask[ 6]|
    mask[79]| mask[70]| mask[61]| mask[52]| mask[43]| mask[34]| mask[25]| mask[16]| mask[ 7]|
    mask[80]| mask[71]| mask[62]| mask[53]| mask[44]| mask[35]| mask[26]| mask[17]| mask[ 8]),
};

const Bitboard pre_prom_area[COLOR_SIZE] = {
  ( mask[75]| mask[66]| mask[57]| mask[48]| mask[39]| mask[30]| mask[21]| mask[12]| mask[ 3] ),
  ( mask[77]| mask[68]| mask[59]| mask[50]| mask[41]| mask[32]| mask[23]| mask[14]| mask[ 5] ),
};
const Bitboard rank3_area[COLOR_SIZE] = {
  (mask[74]| mask[65]| mask[56]| mask[47]| mask[38]| mask[29]| mask[20]| mask[11]| mask[ 2]),
  (mask[78]| mask[69]| mask[60]| mask[51]| mask[42]| mask[33]| mask[24]| mask[15]| mask[ 6]),
};

const Bitboard ALL_ONE_BB =
   mask[72] | mask[63] | mask[54] | mask[45] | mask[36] | mask[27] | mask[18] | mask[ 9] | mask[ 0] |
   mask[73] | mask[64] | mask[55] | mask[46] | mask[37] | mask[28] | mask[19] | mask[10] | mask[ 1] |
   mask[74] | mask[65] | mask[56] | mask[47] | mask[38] | mask[29] | mask[20] | mask[11] | mask[ 2] |
   mask[75] | mask[66] | mask[57] | mask[48] | mask[39] | mask[30] | mask[21] | mask[12] | mask[ 3] |
   mask[76] | mask[67] | mask[58] | mask[49] | mask[40] | mask[31] | mask[22] | mask[13] | mask[ 4] |
   mask[77] | mask[68] | mask[59] | mask[50] | mask[41] | mask[32] | mask[23] | mask[14] | mask[ 5] |
   mask[78] | mask[69] | mask[60] | mask[51] | mask[42] | mask[33] | mask[24] | mask[15] | mask[ 6] |
   mask[79] | mask[70] | mask[61] | mask[52] | mask[43] | mask[34] | mask[25] | mask[16] | mask[ 7] |
   mask[80] | mask[71] | mask[62] | mask[53] | mask[44] | mask[35] | mask[26] | mask[17] | mask[ 8] ;

const Bitboard not_prom_area[COLOR_SIZE] = {
  (mask[75] | mask[66] | mask[57] | mask[48] | mask[39] | mask[30] | mask[21] | mask[12] | mask[ 3] |
   mask[76] | mask[67] | mask[58] | mask[49] | mask[40] | mask[31] | mask[22] | mask[13] | mask[ 4] |
   mask[77] | mask[68] | mask[59] | mask[50] | mask[41] | mask[32] | mask[23] | mask[14] | mask[ 5] |
   mask[78] | mask[69] | mask[60] | mask[51] | mask[42] | mask[33] | mask[24] | mask[15] | mask[ 6] |
   mask[79] | mask[70] | mask[61] | mask[52] | mask[43] | mask[34] | mask[25] | mask[16] | mask[ 7] |
   mask[80] | mask[71] | mask[62] | mask[53] | mask[44] | mask[35] | mask[26] | mask[17] | mask[ 8] ),

  (mask[72] | mask[63] | mask[54] | mask[45] | mask[36] | mask[27] | mask[18] | mask[ 9] | mask[ 0] |
   mask[73] | mask[64] | mask[55] | mask[46] | mask[37] | mask[28] | mask[19] | mask[10] | mask[ 1] |
   mask[74] | mask[65] | mask[56] | mask[47] | mask[38] | mask[29] | mask[20] | mask[11] | mask[ 2] |
   mask[75] | mask[66] | mask[57] | mask[48] | mask[39] | mask[30] | mask[21] | mask[12] | mask[ 3] |
   mask[76] | mask[67] | mask[58] | mask[49] | mask[40] | mask[31] | mask[22] | mask[13] | mask[ 4] |
   mask[77] | mask[68] | mask[59] | mask[50] | mask[41] | mask[32] | mask[23] | mask[14] | mask[ 5] ),
 };
const Bitboard file_mask[FILE_SIZE] = {
  mask[ 0] | mask[ 1] | mask[ 2] | mask[ 3] | mask[ 4] | mask[ 5] | mask[ 6] | mask[ 7] | mask[ 8],
  mask[ 9] | mask[10] | mask[11] | mask[12] | mask[13] | mask[14] | mask[15] | mask[16] | mask[17],
  mask[18] | mask[19] | mask[20] | mask[21] | mask[22] | mask[23] | mask[24] | mask[25] | mask[26],
  mask[27] | mask[28] | mask[29] | mask[30] | mask[31] | mask[32] | mask[33] | mask[34] | mask[35],
  mask[36] | mask[37] | mask[38] | mask[39] | mask[40] | mask[41] | mask[42] | mask[43] | mask[44],
  mask[45] | mask[46] | mask[47] | mask[48] | mask[49] | mask[50] | mask[51] | mask[52] | mask[53],
  mask[54] | mask[55] | mask[56] | mask[57] | mask[58] | mask[59] | mask[60] | mask[61] | mask[62],
  mask[63] | mask[64] | mask[65] | mask[66] | mask[67] | mask[68] | mask[69] | mask[70] | mask[71],
  mask[72] | mask[73] | mask[74] | mask[75] | mask[76] | mask[77] | mask[78] | mask[79] | mask[80],
};
const Bitboard rank_mask[RANK_SIZE] ={
   mask[72] | mask[63] | mask[54] | mask[45] | mask[36] | mask[27] | mask[18] | mask[ 9] | mask[ 0] ,
   mask[73] | mask[64] | mask[55] | mask[46] | mask[37] | mask[28] | mask[19] | mask[10] | mask[ 1] ,
   mask[74] | mask[65] | mask[56] | mask[47] | mask[38] | mask[29] | mask[20] | mask[11] | mask[ 2] ,
   mask[75] | mask[66] | mask[57] | mask[48] | mask[39] | mask[30] | mask[21] | mask[12] | mask[ 3] ,
   mask[76] | mask[67] | mask[58] | mask[49] | mask[40] | mask[31] | mask[22] | mask[13] | mask[ 4] ,
   mask[77] | mask[68] | mask[59] | mask[50] | mask[41] | mask[32] | mask[23] | mask[14] | mask[ 5] ,
   mask[78] | mask[69] | mask[60] | mask[51] | mask[42] | mask[33] | mask[24] | mask[15] | mask[ 6] ,
   mask[79] | mask[70] | mask[61] | mask[52] | mask[43] | mask[34] | mask[25] | mask[16] | mask[ 7] ,
   mask[80] | mask[71] | mask[62] | mask[53] | mask[44] | mask[35] | mask[26] | mask[17] | mask[ 8] ,
};
const Bitboard ALL_ZERO_BB = Bitboard(0,0);
const Bitboard MIDDLE_AREA_BB = mask[ 4] | mask[13] | mask[22] | mask[31]
                              | mask[40] | mask[49] | mask[58] | mask[67] | mask[76];
constexpr int bishop_attack_size = 20224;
constexpr int rook_attack_size  = 512000;

extern Bitboard rook_attack[rook_attack_size];
extern Bitboard bishop_attack[bishop_attack_size];
extern Bitboard king_attack[SQUARE_SIZE];
extern Bitboard gold_attack[COLOR_SIZE][SQUARE_SIZE];
extern Bitboard silver_attack[COLOR_SIZE][SQUARE_SIZE];
extern Bitboard knight_attack[COLOR_SIZE][SQUARE_SIZE];
extern Bitboard lance_attack[COLOR_SIZE][SQUARE_SIZE][128];
extern Bitboard pawn_attack[COLOR_SIZE][SQUARE_SIZE];
extern Bitboard rook_mask[SQUARE_SIZE];
extern Bitboard bishop_mask[SQUARE_SIZE];
extern u64 bishop_offset[SQUARE_SIZE];
extern u64 rook_offset[SQUARE_SIZE];
extern Bitboard pseudo_rook_attack[SQUARE_SIZE];
extern Bitboard pseudo_bishop_attack[SQUARE_SIZE];
extern Bitboard between[SQUARE_SIZE][SQUARE_SIZE];
//                              ↓王手してる側　　↓王手されている側
extern Bitboard pawn_check_table[COLOR_SIZE][SQUARE_SIZE];
extern Bitboard lance_check_table[COLOR_SIZE][SQUARE_SIZE];
extern Bitboard knight_check_table[COLOR_SIZE][SQUARE_SIZE];
extern Bitboard silver_check_table[COLOR_SIZE][SQUARE_SIZE];
extern Bitboard gold_check_table[COLOR_SIZE][SQUARE_SIZE];

constexpr int rook_shift[SQUARE_SIZE] = {
  50, 51, 51, 51, 51, 51, 51, 51, 50,
  51, 52, 52, 52, 52, 52, 52, 52, 50,
  51, 52, 52, 52, 52, 52, 52, 52, 51,
  51, 52, 52, 52, 52, 52, 52, 52, 51,
  51, 52, 52, 52, 52, 52, 52, 52, 51,
  51, 52, 52, 52, 52, 52, 52, 52, 50,
  51, 52, 52, 52, 52, 52, 52, 52, 51,
  51, 52, 52, 52, 52, 52, 52, 52, 51,
  50, 51, 51, 51, 51, 51, 51, 51, 50,
};
constexpr int bishop_shift[SQUARE_SIZE] = {
  57, 58, 58, 58, 58, 58, 58, 58, 57,
  58, 58, 58, 58, 58, 58, 58, 58, 58,
  58, 58, 56, 56, 56, 56, 56, 58, 58,
  58, 58, 56, 54, 54, 54, 56, 58, 58,
  58, 58, 56, 54, 52, 54, 56, 58, 58,
  58, 58, 56, 54, 54, 54, 56, 58, 58,
  58, 58, 56, 56, 56, 56, 56, 58, 58,
  58, 58, 58, 58, 58, 58, 58, 58, 58,
  57, 58, 58, 58, 58, 58, 58, 58, 57
};
constexpr int select_bb[SQUARE_SIZE] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1,
};
constexpr int lance_shift_num[SQUARE_SIZE] = {
   1,  1,  1,  1,  1,  1,  1,  1,  1,
  10, 10, 10, 10, 10, 10, 10, 10, 10,
  19, 19, 19, 19, 19, 19, 19, 19, 19,
  28, 28, 28, 28, 28, 28, 28, 28, 28,
  37, 37, 37, 37, 37, 37, 37, 37, 37,
  46, 46, 46, 46, 46, 46, 46, 46, 46,
  55, 55, 55, 55, 55, 55, 55, 55, 55,
   1,  1,  1,  1,  1,  1,  1,  1,  1,
  10, 10, 10, 10, 10, 10, 10, 10, 10,
};

constexpr u64 rook_magic[SQUARE_SIZE] = {
  2467972605765239040LLU,//0
  369295530223784256LLU,//1
  18015790078906432LLU,//2
  306245328712015936LLU,//3
  108090789644472352LLU,//4
  2431944624758063120LLU,//5
  90072615588200452LLU,//6
  9241387071556288794LLU,//7
  18014472614576258LLU,//8
  20301384977031936LLU,//9
  2306177260815712320LLU,//10
  1170971104668598336LLU,//11
  292804357676605504LLU,//12
  5206196357110431760LLU,//13
  288247973706597412LLU,//14
  18049600330924056LLU,//15
  9241421688456871938LLU,//16
  581105639527809025LLU,//17
  1155208557512163584LLU,//18
  292736209178854656LLU,//19
  22518066869174336LLU,//20
  721701909022380049LLU,//21
  4828984769169850384LLU,//22
  563018708549669LLU,//23
  6953557962103195912LLU,//24
  54043264248447012LLU,//25
  18015016985105698LLU,//26
  4802667998085185LLU,//27
  1152930301303882496LLU,//28
  324276809648590976LLU,//29
  9263922028371558468LLU,//30
  147492956650083088LLU,//31
  72058693684035592LLU,//32
  18017147422770308LLU,//33
  108086494404546564LLU,//34
  306244860694757650LLU,//35
  14564641384181792769LLU,//36
  2332864624157917248LLU,//37
  7226043207455867008LLU,//38
  1152923774531141698LLU,//39
  2882339121985224720LLU,//40
  18295909993676813LLU,//41
  9259401968282501320LLU,//42
  2936347094485827908LLU,//43
  18014591859818498LLU,//44
  9007217642930688LLU,//45
  4611769582158356608LLU,//46
  10376856573019488384LLU,//47
  1441154079798592001LLU,//48
  720577039899296257LLU,//49
  216173331873792513LLU,//50
  72058694623298562LLU,//51
  585679058327568897LLU,//52
  2306001648259958282LLU,//53
  54183950670660628LLU,//54
  2308211976004698372LLU,//55
  148654011069859844LLU,//56
  18122150665781339LLU,//57
  5629792799956993LLU,//58
  36031550228275202LLU,//59
  18032265582350337LLU,//60
  666535218754617349LLU,//61
  73253914304319492LLU,//62
  9277415258258146048LLU,//63
  81069191406289024LLU,//64
  18021064315502720LLU,//65
  9259402075136068482LLU,//66
  126101889086390795LLU,//67
  2395915277712760833LLU,//68
  11889503171959087172LLU,//69
  18014535953678473LLU,//70
  1747401621744124161LLU,//71
  2319459361228866304LLU,//72
  4615151679094915200LLU,//73
  1152930300741813264LLU,//74
  633319981383872LLU,//75
  9224251651030778384LLU,//76
  1196269196283920LLU,//77
  20582857942499332LLU,//78
  1759218942349316LLU,//79
  4538784838680842LLU,//80
};
constexpr u64 bishop_magic[SQUARE_SIZE] = {
  9315696998118596620LLU,//0
  579280177151234083LLU,//1
  2287053979132160LLU,//2
  613053874840797184LLU,//3
  576743086416757248LLU,//4
  9224075999729156097LLU,//5
  2306547109827985538LLU,//6
  18692779827208LLU,//7
  578853452950552836LLU,//8
  148636382263248132LLU,//9
  18305220417990680LLU,//10
  1549275659573028105LLU,//11
  4719772960315949344LLU,//12
  36136276462665761LLU,//13
  3459082004279071232LLU,//14
  2251869086828672LLU,//15
  1152922347502635024LLU,//16
  581034760403574788LLU,//17
  9112769720754188LLU,//18
  9295992598030189057LLU,//19
  4616893310873634820LLU,//20
  4611968603687882754LLU,//21
  9297472473007232LLU,//22
  5348070326813312LLU,//23
  867083945198551556LLU,//24
  2670796241739194432LLU,//25
  324278964516294666LLU,//26
  9592953088248545282LLU,//27
  1585551434399645701LLU,//28
  20338766107509264LLU,//29
  1162511445561708545LLU,//30
  564049545003394LLU,//31
  36310822028968000LLU,//32
  13871650077140976340LLU,//33
  873984269453238273LLU,//34
  1729718845448425988LLU,//35
  4685995695819260048LLU,//36
  37155831335190552LLU,//37
  36170636703310880LLU,//38
  2450099003665221636LLU,//39
  1153203049376797192LLU,//40
  882846539343464449LLU,//41
  11263569182196225LLU,//42
  721139993099371009LLU,//43
  2314885466392888322LLU,//44
  292809850739097608LLU,//45
  3747628622479888512LLU,//46
  36169810794648168LLU,//47
  648672554994892816LLU,//48
  72057628536078401LLU,//49
  38280808896692228LLU,//50
  72103773670017028LLU,//51
  9296848036938514436LLU,//52
  18296973309329442LLU,//53
  2307109964437528592LLU,//54
  2314929924136378368LLU,//55
  108229363003064320LLU,//56
  1729382394357744648LLU,//57
  2594073389677172232LLU,//58
  4639851110434608128LLU,//59
  9011599449293457LLU,//60
  4782965741353698306LLU,//61
  72339688567476736LLU,//62
  2923469547763728394LLU,//63
  2305913395408275584LLU,//64
  11538785354494519360LLU,//65
  36028801385386016LLU,//66
  2476988592491335688LLU,//67
  5188463431161780228LLU,//68
  360340747352813570LLU,//69
  9800681792526615042LLU,//70
  73258398443374604LLU,//71
  292789020621014144LLU,//72
  9385677993068732418LLU,//73
  1306888325593827856LLU,//74
  576478465420364296LLU,//75
  648553573663244548LLU,//76
  5116370720406374924LLU,//77
  4908923602457870510LLU,//78
  4756965078226504712LLU,//79
  4656722291207401474LLU,//80
};

//constexpr int index64_lsb [64] = {
//   0,  1, 48,  2, 57, 49, 28,  3,
//  61, 58, 50, 42, 38, 29, 17,  4,
//  62, 55, 59, 36, 53, 51, 43, 22,
//  45, 39, 33, 30, 24, 18, 12,  5,
//  63, 47, 56, 27, 60, 41, 37, 16,
//  54, 35, 52, 21, 44, 32, 23, 11,
//  46, 26, 40, 15, 34, 20, 31, 10,
//  25, 14, 19,  9, 13,  8,  7,  6,
//};
//
//constexpr int index64_msb[64] = {
//   0, 47,  1, 56, 48, 27,  2, 60,
//  57, 49, 41, 37, 28, 16,  3, 61,
//  54, 58, 35, 52, 50, 42, 21, 44,
//  38, 32, 29, 23, 17, 11,  4, 62,
//  46, 55, 26, 59, 40, 36, 15, 53,
//  34, 51, 20, 43, 31, 22, 10, 45,
//  25, 39, 14, 33, 19, 30,  9, 24,
//  13, 18,  8, 12,  7,  6,  5, 63
//};
//http://chessprogramming.wikispaces.com/BitScan
static inline int lsb_index(const u64 b){
//constexpr u64 debruijn64 = (0x03f79d71b4cb0a89);
  //ASSERT (b != 0);
  //return index64_lsb[((b & -b) * debruijn64) >> 58];
  ASSERT(b!=0);
  return __builtin_ctzll(b);
}
static inline int msb_index(const u64 b){
 // constexpr u64 debruijn64 = 0x03f79d71b4cb0a89ll;
 // u64 bb = b;
 // ASSERT (bb != 0);
 // bb |= bb >> 1;
 // bb |= bb >> 2;
 // bb |= bb >> 4;
 // bb |= bb >> 8;
 // bb |= bb >> 16;
 // bb |= bb >> 32;
 // return index64_msb[(bb * debruijn64) >> 58];
  return __builtin_clzll(b);
}
static inline int pop_cnt_index(const u64 b){
//  auto tmp = b;
 // auto ret = 0;
 // while(tmp){
 //   ++ret;
 //   tmp &= tmp-1;
  //}
  //return ret;
#if 0 //defined(HAVE_SSE4)
  return _mm_popcnt_u64(b);
#else
  return __builtin_popcountll(b);
#endif
}
template<BitOp o>inline Square Bitboard::lsb_right(){

  ASSERT(this->bb(0));
  Square ret = (Square)(lsb_index(this->bb(0)));
  ASSERT(((int)ret >= 0) && ((int)ret <= 62));
  if(o == D){
    bb_[0] &= bb_[0] - 1;
  }
  ASSERT(square_is_ok(ret));
  return ret;
}
template<BitOp o>inline Square Bitboard::lsb_left(){

  ASSERT(this->bb(1));
  Square ret = (Square)(lsb_index(this->bb(1)));
  ASSERT(((int)ret >= 0) && ((int)ret <= 17));
  if(o == D){
    bb_[1] &= bb_[1] - 1;
  }
  ASSERT(square_is_ok(ret+63));
  return (Square)(ret+63);
}
template<BitOp o>inline Square Bitboard::msb_right(){

  ASSERT(this->bb(0));
  Square ret = (Square)(msb_index(this->bb(0)));
  if(o == D){
    bb_[0]^=(1ull)<<ret;
  }
  return ret;
}
template<BitOp o>inline Square Bitboard::msb_left(){

  ASSERT(this->bb(1));
  Square ret = (Square)(msb_index(this->bb(1)));
  if(o == D){
    bb_[1]^=(1ull)<<ret;
  }
  return (Square)(ret+63);
}
inline int Bitboard::pop_cnt() const {
  auto ret = pop_cnt_index(bb(0));
  ret += pop_cnt_index(bb(1));
  return ret;
}
inline bool Bitboard::is_seted(const Square sq) const {
  ASSERT(square_is_ok(sq));
  return and_is_not_zero(mask[sq]);
}
inline void Bitboard::And(const Square sq) {
  ASSERT(square_is_ok(sq));
 *this &= mask[sq];
}
inline void Bitboard::Or(const Square sq) {
  ASSERT(square_is_ok(sq));
  *this |= mask[sq];
}
inline void Bitboard::Xor(const Square sq) {
  ASSERT(square_is_ok(sq));
  *this ^= mask[sq];
}
static inline Bitboard get_pawn_attack(const Color c, const Square from){
  ASSERT(color_is_ok(c));
  ASSERT(square_is_ok(from));
  return pawn_attack[c][from];
}
template<Color c>static inline Bitboard get_pawn_attack(const Bitboard & from){
  ASSERT(color_is_ok(c));
  return (c == BLACK) ? from >>1 : from << 1;
}
static inline Bitboard get_pawn_attack(const Color c, const Bitboard & from){
  ASSERT(color_is_ok(c));
  return (c == BLACK) ? get_pawn_attack<BLACK>(from) : get_pawn_attack<WHITE>(from);
}
static inline Bitboard get_knight_attack(const Color c, const Square from){
  ASSERT(color_is_ok(c));
  ASSERT(square_is_ok(from));
  return knight_attack[c][from];
}
static inline Bitboard get_silver_attack(const Color c, const Square from){
  ASSERT(color_is_ok(c));
  ASSERT(square_is_ok(from));
  return silver_attack[c][from];
}
static inline Bitboard get_gold_attack(const Color c, const Square from){
  ASSERT(color_is_ok(c));
  ASSERT(square_is_ok(from));
  return gold_attack[c][from];
}
static inline Bitboard get_king_attack(const Square from){
  ASSERT(square_is_ok(from));
  return king_attack[from];
}
static inline Bitboard get_lance_attack(const Color c, const Square from, const Bitboard & occ){
  ASSERT(color_is_ok(c));
  ASSERT(square_is_ok(from));
  return lance_attack[c][from][((occ.bb(select_bb[from]))>>lance_shift_num[from])&127];
}
static inline u64 get_lance_attack(const Color c, const Square from, const int select, const Bitboard & occ){
  ASSERT(color_is_ok(c));
  ASSERT(square_is_ok(from));
  ASSERT(select == 0 || select == 1);
  return lance_attack[c][from][((occ.bb(select))>>lance_shift_num[from])&127].bb(select);
}
static inline Bitboard get_bishop_attack(const Square from, const Bitboard & occ){
  ASSERT(square_is_ok(from));
  const Bitboard tmp = occ & bishop_mask[from];
  const u64 index = (tmp.merge() * bishop_magic[from]) >> bishop_shift[from];
  return bishop_attack[bishop_offset[from] + index];
}
static inline Bitboard get_rook_attack(const Square from, const Bitboard & occ){
  ASSERT(square_is_ok(from));
  const Bitboard tmp = occ & rook_mask[from];
  const u64 index = (tmp.merge() * rook_magic[from]) >> rook_shift[from];
  return rook_attack[rook_offset[from] + index];
}
static inline Bitboard get_pbishop_attack(const Square from, const Bitboard & occ){
  ASSERT(square_is_ok(from));
  return get_bishop_attack(from,occ) | king_attack[from];
}
static inline Bitboard get_prook_attack(const Square from, const Bitboard & occ){
  ASSERT(square_is_ok(from));
  return get_rook_attack(from,occ) | king_attack[from];
}
static inline Bitboard get_prom_area(const Color c){
  ASSERT(color_is_ok(c));
  return prom_area[c];
}
static inline Bitboard get_pre_prom_area(const Color c){
  ASSERT(color_is_ok(c));
  return pre_prom_area[c];
}
static inline Bitboard get_not_prom_area(const Color c){
  ASSERT(color_is_ok(c));
  return not_prom_area[c];
}
static inline Bitboard get_middle_area(){
  return MIDDLE_AREA_BB;
}
static inline Bitboard get_rank3_area(const Color c){
  ASSERT(color_is_ok(c));
  return rank3_area[c];
}
static inline Bitboard get_between(const Square sq1, const Square sq2){
  ASSERT(square_is_ok(sq1));
  ASSERT(square_is_ok(sq2));
  return between[sq1][sq2];
}
static inline Bitboard get_pawn_check_table(const Color c, const Square sq){
  ASSERT(color_is_ok(c));
  ASSERT(square_is_ok(sq));
  return pawn_check_table[c][sq];
}
static inline Bitboard get_lance_check_table(const Color c, const Square sq){
  ASSERT(color_is_ok(c));
  ASSERT(square_is_ok(sq));
  return lance_check_table[c][sq];
}
static inline Bitboard get_knight_check_table(const Color c, const Square sq){
  ASSERT(color_is_ok(c));
  ASSERT(square_is_ok(sq));
  return knight_check_table[c][sq];
}
static inline Bitboard get_silver_check_table(const Color c, const Square sq){
  ASSERT(color_is_ok(c));
  ASSERT(square_is_ok(sq));
  return silver_check_table[c][sq];
}
static inline Bitboard get_gold_check_table(const Color c, const Square sq){
  ASSERT(color_is_ok(c));
  ASSERT(square_is_ok(sq));
  return gold_check_table[c][sq];
}
static inline Bitboard get_pseudo_attack(const PieceType pt, const Color c, const Square from){

  ASSERT(color_is_ok(c));
  ASSERT(square_is_ok(from));
  switch(pt){
    case PAWN:
      return get_pawn_attack(c,from);
    case KNIGHT:
      return get_knight_attack(c,from);
    case LANCE:
      return lance_attack[c][from][0];
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
      return pseudo_bishop_attack[from];
    case ROOK:
      return pseudo_rook_attack[from];
    case PBISHOP:
      return pseudo_bishop_attack[from] | get_king_attack(from);
    case PROOK:
      return pseudo_rook_attack[from] | get_king_attack(from);
    default:
      ASSERT(false);
      return Bitboard(0,0);
  }
}
template<PieceType pt>static inline
Bitboard get_pseudo_attack(const Color c, const Square from){

  ASSERT(color_is_ok(c));
  ASSERT(square_is_ok(from));
  switch(pt){
    case PAWN:
      return get_pawn_attack(c,from);
    case KNIGHT:
      return get_knight_attack(c,from);
    case LANCE:
      return lance_attack[c][from][0];
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
      return pseudo_bishop_attack[from];
    case ROOK:
      return pseudo_rook_attack[from];
    case PBISHOP:
      return pseudo_bishop_attack[from] | get_king_attack(from);
    case PROOK:
      return pseudo_rook_attack[from] | get_king_attack(from);
    default:
      ASSERT(false);
      return Bitboard(0,0);
  }
}
extern void init_bitboard();
extern void find_magic();

#define BB_FOR_EACH(bbb,sq,XXX) while(bbb.bb(0)){\
                                  sq = bbb.lsb_right<D>();\
                                  XXX;\
                                }\
                                while(bbb.bb(1)){\
                                  sq = bbb.lsb_left<D>();\
                                  XXX;\
                                }\

#endif
