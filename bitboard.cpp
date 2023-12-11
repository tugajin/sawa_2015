#include "bitboard.h"
#include <iostream>
#include <bitset>
#include <map>
#include <boost/static_assert.hpp>
#include <boost/format.hpp>

Bitboard rook_attack[rook_attack_size];
Bitboard bishop_attack[bishop_attack_size];
Bitboard king_attack[SQUARE_SIZE];
Bitboard gold_attack[COLOR_SIZE][SQUARE_SIZE];
Bitboard silver_attack[COLOR_SIZE][SQUARE_SIZE];
Bitboard knight_attack[COLOR_SIZE][SQUARE_SIZE];
Bitboard lance_attack[COLOR_SIZE][SQUARE_SIZE][128];
Bitboard pawn_attack[COLOR_SIZE][SQUARE_SIZE];
Bitboard rook_mask[SQUARE_SIZE];
Bitboard bishop_mask[SQUARE_SIZE];

Bitboard between[SQUARE_SIZE][SQUARE_SIZE];

u64 rook_offset[SQUARE_SIZE];
u64 bishop_offset[SQUARE_SIZE];

Bitboard pseudo_rook_attack[SQUARE_SIZE];
Bitboard pseudo_bishop_attack[SQUARE_SIZE];


//                         ↓王手してる側　　↓王手されている側
Bitboard pawn_check_table[COLOR_SIZE][SQUARE_SIZE];
Bitboard lance_check_table[COLOR_SIZE][SQUARE_SIZE];
Bitboard knight_check_table[COLOR_SIZE][SQUARE_SIZE];
Bitboard silver_check_table[COLOR_SIZE][SQUARE_SIZE];
Bitboard gold_check_table[COLOR_SIZE][SQUARE_SIZE];

static Bitboard index_to_occ(const int index,
                              const int bits,
                              const Bitboard & mask);

static Bitboard occ_to_attack(const Bitboard & occ,
                              const File file,
                              const Rank rank,
                              const bool is_bishop);
#if 0
void find_bishop_rook_magic(const File file,
                            const Rank rank,
                            const bool is_bishop);
#endif

void Bitboard::print()const{
  std::cout<<"-----------------"<<std::endl;
  for(int rank = 0; rank < 9; rank++){
    for(int file = 1; file >= 0; file--){//left
      int xy = rank + file * 9;
      if( bb_[1] & (1ULL<<xy) ){
          std::cout<<"1,";
      }else{
          std::cout<<"0,";
      }
    }
    for(int file = 6; file >= 0; file--){//right
      int xy = rank + file * 9;
      if( bb_[0] & (1ULL<<xy) ){
          std::cout<<"1,";
      }else{
        std::cout<<"0,";
      }
    }
    std::cout<<"\n";
  }
  std::cout<<"-----------------"<<std::endl;
}
void init_bitboard(){

  //std::cout<<"start init\n";
  //init rook_mask, bisbop_mask
  for(int rank = 0; rank < 9; rank++){
    for(int file = 0; file < 9; file++){
      //rook_mask
      int xy = file * 9 + rank;
      rook_mask[xy].set(0ull,0ull);
      //left mark
      for(int file2 = file + 1; file2 <=7; file2++){
        int xy2 = file2 * 9 + rank;
        rook_mask[xy].Or((Square)xy2);
      }
      //right mark
      for(int file2 = file - 1; file2 >=1; file2--){
        int xy2 = file2 * 9 + rank;
        rook_mask[xy].Or((Square)xy2);
      }
      //up mark
      for(int rank2 = rank - 1; rank2 >=1; rank2--){
        int xy2 = file * 9 + rank2;
        rook_mask[xy].Or((Square)xy2);
      }
      //down mark
      for(int rank2 = rank + 1; rank2 <=7; rank2++){
        int xy2 = file * 9 + rank2;
        rook_mask[xy].Or((Square)xy2);
      }
      //bishop_mask
      xy = file * 9 + rank;

      bishop_mask[xy].set(0ull,0ull);
      //right up mark
      for(int file2 = file - 1, rank2 = rank - 1; file2 >= 1 && rank2 >= 1; file2--,rank2--){
        int xy2 = file2 * 9 + rank2;
        bishop_mask[xy].Or((Square)xy2);
      }
      //left up mark
      for(int file2 = file + 1, rank2 = rank - 1; file2 <= 7 && rank2 >= 1; file2++, rank2--){
        int xy2 = file2 * 9 + rank2;
        bishop_mask[xy].Or((Square)xy2);
      }
      //right down mark
      for(int file2 = file - 1, rank2 = rank + 1; file2 >= 1 && rank2 <= 7; file2--, rank2++){
        int xy2 = file2 * 9 + rank2;
        bishop_mask[xy].Or((Square)xy2);
      }
      //left down mark
      for(int file2 = file + 1, rank2 = rank + 1; file2 <= 7 && rank2 <= 7; file2++, rank2++){
        int xy2 = file2 * 9 + rank2;
        bishop_mask[xy].Or((Square)xy2);
      }
    }
  }
  //gold silver knight pawn attack
  for(int rank = 0; rank < 9; rank++){
    for(int file = 0; file < 9; file++){
      int xy = file * 9 + rank;
      int xy2;
      int file2, rank2;

      //init
      gold_attack[BLACK][xy].set(0,0);
      gold_attack[WHITE][xy].set(0,0);
      silver_attack[BLACK][xy].set(0,0);
      silver_attack[WHITE][xy].set(0,0);
      knight_attack[BLACK][xy].set(0,0);
      knight_attack[WHITE][xy].set(0,0);
      pawn_attack[BLACK][xy].set(0,0);
      pawn_attack[WHITE][xy].set(0,0);
      king_attack[xy].set(0,0);

      //left
      file2 = file - 1; rank2 = rank;
      if(file2 >= 0){
        xy2 = file2 * 9 + rank2;
        gold_attack[BLACK][xy].Or((Square)xy2);
        gold_attack[WHITE][xy].Or((Square)xy2);
      }
      //left up
      file2 = file - 1; rank2 = rank - 1;
      if(file2 >= 0 && rank2 >= 0){
        xy2 = file2 * 9 + rank2;
        gold_attack[BLACK][xy].Or((Square)xy2);
        silver_attack[BLACK][xy].Or((Square)xy2);
        silver_attack[WHITE][xy].Or((Square)xy2);
      }
      //left up knight
      file2 = file - 1; rank2 = rank - 2;
      if(file2 >= 0 && rank2 >= 0){
        xy2 = file2 * 9 + rank2;
        knight_attack[BLACK][xy].Or((Square)xy2);
      }
      //up
      file2 = file; rank2 = rank-1;
      if(rank2 >= 0){
        xy2 = file2 * 9 + rank2;
        gold_attack[BLACK][xy].Or((Square)xy2);
        gold_attack[WHITE][xy].Or((Square)xy2);
        silver_attack[BLACK][xy].Or((Square)xy2);
        pawn_attack[BLACK][xy].Or((Square)xy2);
      }
      //right up
      file2 = file + 1; rank2 = rank - 1;
      if(file2 <= 8 && rank2 >= 0){
        xy2 = file2 * 9 + rank2;
        gold_attack[BLACK][xy].Or((Square)xy2);
        silver_attack[BLACK][xy].Or((Square)xy2);
        silver_attack[WHITE][xy].Or((Square)xy2);
      }
      // right up knight
      file2 = file + 1; rank2 = rank - 2;
      if(rank2 >= 0 && file2 <= 8){
        xy2 = file2 * 9 + rank2;
        knight_attack[BLACK][xy].Or((Square)xy2);
      }
      //right
      file2 = file + 1; rank2 = rank;
      if(file2 <= 8){
        xy2 = file2 * 9 + rank2;
        gold_attack[BLACK][xy].Or((Square)xy2);
        gold_attack[WHITE][xy].Or((Square)xy2);
      }
      //right down
      file2 = file + 1; rank2 = rank + 1;
      if(file2 <= 8 && rank2 <= 8){
        xy2 = file2 * 9 + rank2;
        gold_attack[WHITE][xy].Or((Square)xy2);
        silver_attack[BLACK][xy].Or((Square)xy2);
        silver_attack[WHITE][xy].Or((Square)xy2);
      }
      //right down knight
      file2 = file + 1; rank2 = rank + 2;
      if(file2 <= 8 && rank2 <= 8){
        xy2 = file2 * 9 + rank2;
        knight_attack[WHITE][xy].Or((Square)xy2);
      }
      //down
      file2 = file ; rank2 = rank + 1;
      if(rank2 <= 8){
        xy2 = file2 * 9 + rank2;
        gold_attack[BLACK][xy].Or((Square)xy2);
        gold_attack[WHITE][xy].Or((Square)xy2);
        silver_attack[WHITE][xy].Or((Square)xy2);
        pawn_attack[WHITE][xy].Or((Square)xy2);
      }
      //left down
      file2 = file - 1; rank2 = rank + 1;
      if(file2 >= 0 && rank2 <= 8){
        xy2 = file2 * 9 + rank2;
        gold_attack[WHITE][xy].Or((Square)xy2);
        silver_attack[BLACK][xy].Or((Square)xy2);
        silver_attack[WHITE][xy].Or((Square)xy2);
      }
      //left down knight
      file2 = file - 1; rank2 = rank + 2;
      if(rank2 <= 8 && file2 >= 0){
        xy2 = file2 * 9 + rank2;
        knight_attack[WHITE][xy].Or((Square)xy2);
      }
      king_attack[xy] = gold_attack[BLACK][xy] | silver_attack[BLACK][xy];
    }
  }
  for(auto sq = A1; sq < SQUARE_SIZE; sq++){
    bishop_attack[sq].set(0ull,0ull);
    rook_attack[sq].set(0ull,0ull);
    rook_offset[sq] = 0;
    bishop_offset[sq] = 0;
    pseudo_rook_attack[sq].set(0ull,0ull);
    pseudo_bishop_attack[sq].set(0ull,0ull);
  }
  //gen rook bishop attack
  u64 maxb,maxr;
  maxb = maxr = 0;
  for(File file = FILE_A; file < FILE_SIZE; file++){
    for(Rank rank = RANK_1; rank < RANK_SIZE; rank++){
      //bishop
      const auto sq = make_square(file,rank);
      auto bit = bishop_mask[sq].pop_cnt();
      for(int index = 0; index < (1 << bit); index++){
        Bitboard occ = index_to_occ(index,bit,bishop_mask[sq]);
        Bitboard att = occ_to_attack(occ,file,rank,true);
        u64 id = (occ.merge() * bishop_magic[sq]) >> bishop_shift[sq];
        id += bishop_offset[sq];
        maxb = std::max(id,maxb);
        ASSERT(id < 22000);
        bishop_attack[id] = att;
        if(index == 0){
          pseudo_bishop_attack[sq] = att;
        }
      }
      if(sq+1 < SQUARE_SIZE){
        bishop_offset[sq+1] = maxb + 1;
      }
      //rook
      bit = rook_mask[sq].pop_cnt();
      for(int index = 0; index < (1 << bit); index++){
        Bitboard occ = index_to_occ(index,bit,rook_mask[sq]);
        Bitboard att = occ_to_attack(occ,file,rank,false);
        u64 id = (occ.merge() * rook_magic[sq]) >> rook_shift[sq];
        id += rook_offset[sq];
        maxr = std::max(id,maxr);
        ASSERT(id < 600000);
        rook_attack[id] = att;
        if(index == 0){
          pseudo_rook_attack[sq] = att;
        }
      }
      if(sq + 1 < SQUARE_SIZE){
        rook_offset[sq+1] = maxr + 1;
      }
    }
  }
  //gen lance attack
  FILE_FOREACH(file){
    RANK_FOREACH(rank){
      auto xy = make_square(file,rank);
      for(int index = 0; index < 128; index++){
        int pattern = index << 1;
        lance_attack[BLACK][xy][index].set(0,0);
        lance_attack[WHITE][xy][index].set(0,0);
        for(Rank rank2 = rank + RANK_UP; rank_is_ok(rank2); rank2 += RANK_UP){
          auto xy2 = make_square(file,rank2);
          lance_attack[BLACK][xy][index].Or((Square)xy2);
          if(pattern & (1<<rank2)){
            break;
          }
        }
        for(Rank rank2 = rank + RANK_DOWN; rank_is_ok(rank2); rank2 += RANK_DOWN){
          auto xy2 = make_square(file,rank2);
          lance_attack[WHITE][xy][index].Or((Square)xy2);
          if(pattern & (1<<rank2)){
            break;
          }
        }
      }
    }
  }
  //init between
  SQUARE_FOREACH(sq1){
    SQUARE_FOREACH(sq2){
      between[sq1][sq2].set(0,0);
      if((get_pseudo_attack<BISHOP>(BLACK,sq1)
          | get_pseudo_attack<ROOK>(BLACK,sq1))
          .is_seted(sq2)){
        auto file1 = square_to_file(sq1);
        auto file2 = square_to_file(sq2);
        auto rank1 = square_to_rank(sq1);
        auto rank2 = square_to_rank(sq2);
        Square inc = A1;
        BOOST_STATIC_ASSERT(A1 == 0);
        if(file1 < file2){
          inc += LEFT;
        }else if(file1 > file2){
          inc += RIGHT;
        }
        if(rank1 < rank2){
          inc += DOWN;
        }else if(rank1 > rank2){
          inc += UP;
        }
        for(auto sq3 = sq1 + inc; sq3 != sq2; sq3 += inc ){
          ASSERT(square_is_ok(sq3));
          between[sq1][sq2].Or(sq3);
        }
      }
    }
  }
  //init check table
  COLOR_FOREACH(sd){
    SQUARE_FOREACH(king_sq){
      const auto xd = opp_color(sd);
      //std::cout<<"sd "<<sd<<" sq "<<king_sq<<std::endl;
      //gold
      {
        gold_check_table[sd][king_sq].set(0,0);
        Bitboard bb = gold_attack[xd][king_sq];
        while(bb.is_not_zero()){
          const auto to = bb.lsb<D>();
          gold_check_table[sd][king_sq] |= gold_attack[xd][to];
        }
      }
       // std::cout<<"gold\n";
       // gold_check_table[sd][king_sq].print();
      //silver noprom
      {
        silver_check_table[sd][king_sq].set(0,0);
        Bitboard bb = silver_attack[xd][king_sq];
        while(bb.is_not_zero()){
          const auto to = bb.lsb<D>();
          silver_check_table[sd][king_sq] |= silver_attack[xd][to];
        }
      }
      //silver noprom -> prom
      {
        Bitboard bb = gold_attack[xd][king_sq];
        while(bb.is_not_zero()){
          const auto to = bb.lsb<D>();
          if(is_prom_square(to,sd)){
            silver_check_table[sd][king_sq] |= silver_attack[xd][to];
          }
        }
      }
      //silver prom -> noprom
      //       prom -> prom
      {
        Bitboard bb = gold_attack[xd][king_sq];
        while(bb.is_not_zero()){
          const auto to = bb.lsb<D>();
          silver_check_table[sd][king_sq]
            |= silver_attack[xd][to] & prom_area[sd];
        }
      }
      // std::cout<<"silver\n";
      // silver_check_table[sd][king_sq].print();
      //knight noprom
      {
        knight_check_table[sd][king_sq].set(0,0);
        Bitboard bb = knight_attack[xd][king_sq];
        while(bb.is_not_zero()){
          const auto to = bb.lsb<D>();
          knight_check_table[sd][king_sq] |= knight_attack[xd][to];
        }
      }
      //knight noprom -> prom
      {
        Bitboard bb = gold_attack[xd][king_sq];
        while(bb.is_not_zero()){
          const auto to = bb.lsb<D>();
          if(is_prom_square(to,sd)){
            knight_check_table[sd][king_sq] |= knight_attack[xd][to];
          }
        }
      }
      // std::cout<<"knight\n";
      // knight_check_table[sd][king_sq].print();
      //lance
      {
        lance_check_table[sd][king_sq].set(0,0);
        lance_check_table[sd][king_sq] |= lance_attack[xd][king_sq][0];
      }
      //lance noprom -> prom
      //        prom -> prom
      {
        Bitboard bb = gold_attack[xd][king_sq];
        while(bb.is_not_zero()){
          const auto to = bb.lsb<D>();
          if(is_prom_square(to,sd)){
            lance_check_table[sd][king_sq] |= lance_attack[xd][to][0];
          }
        }
      }
      // std::cout<<"lance\n";
      // lance_check_table[sd][king_sq].print();
      //pawn
      {
        pawn_check_table[sd][king_sq].set(0,0);
        Bitboard bb = pawn_attack[xd][king_sq];
        while(bb.is_not_zero()){
          const auto to = bb.lsb<D>();
          pawn_check_table[sd][king_sq] |= pawn_attack[xd][to];
        }
      }
      //pawn noprom -> prom
      //       prom -> prom
      {
        Bitboard bb = gold_attack[xd][king_sq];
        while(bb.is_not_zero()){
          const auto to = bb.lsb<D>();
          if(is_prom_square(to,sd)){
            pawn_check_table[sd][king_sq] |= pawn_attack[xd][to];
          }
        }
      }
      // std::cout<<"pawn\n";
      // pawn_check_table[sd][king_sq].print();
    }
  }

     ASSERT(ALL_ONE_BB.pop_cnt() == SQUARE_SIZE);
     ASSERT(ALL_ZERO_BB.pop_cnt() == 0);
}
static Bitboard index_to_occ( const int index,
                              const int bits,
                              const Bitboard & mask){
  Bitboard tmp = mask;
  Bitboard ret(0ull,0ull);
  for(int i = 0; i < bits; i++){
    Square sq = tmp.lsb<D>();
    if(index & (1 << i)){
      ASSERT(square_is_ok(sq));
      ret.Or(sq);
    }
  }
  return ret;
}
static Bitboard set_each_dir_attack(const Bitboard & occ,
                                    const File file,
                                    const Rank rank,
                                    const File inc_file,
                                    const Rank inc_rank){
  ASSERT(file_is_ok(file));
  ASSERT(rank_is_ok(rank));
  File file2;
  Rank rank2;
  Bitboard ret_bb(0ull,0ull);
  for(file2 = file + inc_file,rank2 = rank + inc_rank
      ; (file_is_ok(file2) && rank_is_ok(rank2))
      ; file2 += inc_file,rank2 += inc_rank){
    Square xy2 = make_square(file2,rank2);
    ASSERT(square_is_ok(xy2));
    ret_bb.Or(xy2);
    if(occ.is_seted(xy2)){
      break;
    }
  }
 return ret_bb;
}
static Bitboard occ_to_attack(const Bitboard & occ,
                              const File file,
                              const Rank rank,
                              const bool is_bishop){
  Bitboard ret_attack(0ull,0ull);
  if(is_bishop){
    ret_attack |= set_each_dir_attack(occ,file,rank,FILE_LEFT,  RANK_UP);
    ret_attack |= set_each_dir_attack(occ,file,rank,FILE_RIGHT, RANK_UP);
    ret_attack |= set_each_dir_attack(occ,file,rank,FILE_LEFT,  RANK_DOWN);
    ret_attack |= set_each_dir_attack(occ,file,rank,FILE_RIGHT, RANK_DOWN);
  }else{
    //hack assume  RANK_1 = 0, FILE_A = 0
    ret_attack |= set_each_dir_attack(occ,file,rank,FILE_LEFT,  RANK_1 );
    ret_attack |= set_each_dir_attack(occ,file,rank,FILE_RIGHT, RANK_1);
    ret_attack |= set_each_dir_attack(occ,file,rank,FILE_A,     RANK_UP);
    ret_attack |= set_each_dir_attack(occ,file,rank,FILE_A,     RANK_DOWN);
  }
  return ret_attack;
}
#if 0
void find_magic(){
  printf("constexpr u64 rook_magic[SQUARE_SIZE] = { \n");
  for(File file = FILE_A; file < FILE_SIZE; file++){
    for(Rank rank = RANK_1; rank < RANK_SIZE; rank++){
      find_bishop_rook_magic(file,rank,false);
      printf(",//%d\n",make_square(file,rank));
    }
  }
  printf("};\n");
  printf("constexpr u64 bishop_magic[SQUARE_SIZE] = { \n");
  for(File file = FILE_A; file < FILE_SIZE; file++){
    for(Rank rank = RANK_1; rank < RANK_SIZE; rank++){
      find_bishop_rook_magic(file,rank,true);
      printf(",//%d\n",make_square(file,rank));
    }
  }
  printf("};\n");
}
void find_bishop_rook_magic(const File file,
                           const Rank rank,
                           const bool is_bishop){
  constexpr int size = 200000;
  constexpr int try_max = 500000000;
  static Bitboard occ[size];
  static Bitboard attack[size];
  static Bitboard used[size];
  //gen attack
  auto sq = make_square(file,rank);
  //rook make occ and attack
  auto bit_num = is_bishop
                  ? bishop_mask[sq].pop_cnt()
                  : rook_mask[sq].pop_cnt();
  auto range = 1 << bit_num;
  for(int index = 0; index < range; index++){
    const Bitboard wrap_mask = is_bishop ? bishop_mask[sq]
                                         : rook_mask[sq];
    occ[index] = index_to_occ(index,bit_num,wrap_mask);
    attack[index] = occ_to_attack(occ[index],file,rank,is_bishop);
  }
  //find magic
  bool flag;
  u64 magic;
  for(int try_num = 0; try_num < try_max; try_num++){
    magic = random_for_magic();
    for(int i = 0; i < size; i++){
      used[i].set(0ull,0ull);
    }
    flag = true;
    u64 max_id = 0ull;
    for(int index = 0; index < range; index++){
      const int wrap_shift = is_bishop ? bishop_shift[sq]
                                       : rook_shift[sq];
      u64 id = (occ[index].merge() * magic) >> wrap_shift;
      max_id = std::max(max_id,id);
      if(!used[id].is_not_zero()){
        used[id] = attack[index];
      }else if(used[id] == attack[index]){
        //do nothing
      }else{
        flag = false;
        break;
      }
    }//index
    if(flag){
      break;
    }
  }//try_num
  printf("%llu",magic);
}
#endif
