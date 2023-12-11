#include "type.h"
#include "bitboard.h"

SquareRelation square_relation[SQUARE_SIZE][SQUARE_SIZE];

void init_relation(){

  SQUARE_FOREACH(sq1){
    SQUARE_FOREACH(sq2){
      square_relation[sq1][sq2] = NONE_RELATION;
      if(get_pseudo_attack<BISHOP>(BLACK,sq1).is_seted(sq2)){
        const auto rank1 = square_to_rank(sq1);
        const auto file1 = square_to_file(sq1);
        const auto rank2 = square_to_rank(sq2);
        const auto file2 = square_to_file(sq2);
        if((rank1 > rank2 && file1 > file2)
        ||(rank1 < rank2 && file1 < file2)){
          square_relation[sq1][sq2] = RIGHT_UP_RELATION;
        }else{
          ASSERT((rank1 < rank2 && file1 > file2)
               ||(rank1 > rank2 && file1 < file2));
          square_relation[sq1][sq2] = LEFT_UP_RELATION;
        }
      }else if(get_pseudo_attack<ROOK>(BLACK,sq1).is_seted(sq2)){
        if(square_to_rank(sq1) == square_to_rank(sq2)){
          square_relation[sq1][sq2] = RANK_RELATION;
        }else{
          ASSERT(square_to_file(sq1) == square_to_file(sq2));
          square_relation[sq1][sq2] = FILE_RELATION;
        }
      }
    }
  }
}
bool value_is_ok(const Value v){

  if(v > VALUE_INF){
    printf("too big %d\n",v);
    return false;
  }
  if(v < -VALUE_INF){
    printf("too small %d\n",v);
    return false;
  }
  return true;
}
