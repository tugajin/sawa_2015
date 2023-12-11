#include "check_info.h"
#include "position.h"

CheckInfo::CheckInfo(const Position & pos){

  //ASSERT(pos.is_ok());
  const auto me = pos.turn();
  const auto opp = opp_color(me);
  pinned_ = pos.pinned_piece(me);
  king_sq_ = pos.king_sq(opp);
  check_sq_[ROOK] = pos.attack_from<ROOK>(opp,king_sq_);
  check_sq_[BISHOP] = pos.attack_from<BISHOP>(opp,king_sq_);
  check_sq_[GOLD] = pos.attack_from<GOLD>(opp,king_sq_);
  check_sq_[SILVER] = pos.attack_from<SILVER>(opp,king_sq_);
  check_sq_[KNIGHT] = pos.attack_from<KNIGHT>(opp,king_sq_);
  check_sq_[LANCE] = check_sq_[ROOK] & get_pseudo_attack<LANCE>(opp,king_sq_);
  check_sq_[PAWN] = pos.attack_from<PAWN>(opp,king_sq_);
  check_sq_[PROOK] = check_sq_[ROOK] | pos.attack_from<KING>(opp,king_sq_);
  check_sq_[PBISHOP] = check_sq_[BISHOP] | pos.attack_from<KING>(opp,king_sq_);
  check_sq_[PSILVER] = check_sq_[PLANCE] = check_sq_[PKNIGHT] = check_sq_[PPAWN] = check_sq_[GOLD];
  check_sq_[EMPTY_T].set(0,0);
  check_sq_[KING] = pos.attack_from<KING>(opp,king_sq_);
  discover_checker_ = pos.discover_checker();
}
void CheckInfo::dump()const{
 
  printf("pinned\n");
  pinned().print();
  printf("king %d\n",king_sq());
  printf("discover\n");
  discover_checker().print();
  printf("check sq\n");
  for(PieceType pt = PPAWN; pt < PIECE_TYPE_SIZE; pt++){
    std::cout<<str_piece_type_name(pt)<<std::endl;
    check_sq(pt).print();
  }
}
bool CheckInfo::is_ok()const{

 if(!square_is_ok(king_sq_)){
    printf("ch square error \n");
    dump();
    return false;
  }
  if(pinned_.pop_cnt() > 5){
    printf("ch pinned \n");
    dump();
    return false;
  }
  if(discover_checker_.pop_cnt() > 5){
    printf("ch discover \n");
    dump();
    return false;
  }
  return true;
}

