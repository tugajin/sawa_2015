#include "myboost.h"
#include "move.h"
#include "position.h"
#include <boost/lexical_cast.hpp>

bool Move::is_ok()const{

  const auto from = this->from();
  const auto to = this->to();

  if(this->value() == 0){
    std::cout<<"move none\n";
    std::cout<<this->from()<<" "<<this->to()<<std::endl;
    return false;
  }
  if(this->value() == move_pass().value()){
    std::cout<<"move pass\n";
    std::cout<<this->from()<<" "<<this->to()<<std::endl;
    return false;
  }

  if(!square_is_ok(to)){
    std::cout<<"square error\n";
    std::cout<<this->from()<<" "<<this->to()<<std::endl;
    return false;
  }
  if(square_is_drop(from)){
    const auto hp = this->hand_piece();
    if(!hand_piece_is_ok(hp)){
      std::cout<<"hp error\n";
      std::cout<<this->from()<<" "<<this->to()<<" "<<hp<<std::endl;
      return false;
    }
  }
  return true;
}
bool Move::is_ok(const Position & pos)const{

  if(!is_ok()){
    return false;
  }

  const auto me = pos.turn();
  const auto opp = opp_color(me);
  const auto from = this->from();
  const auto to = this->to();
  if(square_is_drop(from)){
    const auto bb = pos.piece_bb(me,PAWN);
    if((file_mask[square_to_file(to)] &bb).is_not_zero() ){
      if(hand_piece() == PAWN_H){
#ifdef LEARN
        printf("double pawn %d %d\n",to,square_to_file(to));
        pos.dump();
        std::cout<<"from "<<from<<" to "<<to<<std::endl;
#endif
        return false;
      }
    }
    if(pos.square(to) != EMPTY){
      printf("exist pawn\n");
      pos.dump();
      std::cout<<"from "<<from<<" to "<<to<<std::endl;
      return false;
    }
  }else{

    const auto p = pos.square(from);
    const auto pt = piece_to_piece_type(p);
    const auto cap = pos.square(to);
    const auto prom = this->is_prom();
    const auto from_prom = get_prom_area(me).is_seted(from);
    const auto to_prom = get_prom_area(me).is_seted(to);

    if(piece_color(p) != me){
      printf("opp piece move\n");
      pos.dump();
      std::cout<<"from "<<from<<" to "<<to<<" prom "<<prom<<std::endl;
      return false;
    }
    if(cap != EMPTY && piece_color(cap) != opp){
      printf("cap my piece\n");
      pos.dump();
      std::cout<<"from "<<from<<" to "<<to<<" prom "<<prom<<std::endl;
      return false;
    }
    if(prom && !can_prom_piece(p)){
      printf("can not prom piece prom\n");
      pos.dump();
      std::cout<<"from "<<from<<" to "<<to<<" prom "<<prom<<std::endl;
      return false;
    }
    if(prom && !from_prom
        && !to_prom){
      printf("can not prom area \n");
      pos.dump();
      std::cout<<"from "<<from<<" to "<<to<<" prom "<<prom<<std::endl;
      return false;
    }
    if(!is_ok_bonanza(pos)){
      printf("rook bishop lance pawn must prom\n");
      printf("%d\n",pt);
      pos.dump();
      std::cout<<"from "<<from<<" to "<<to<<" prom "<<prom<<std::endl;
      return false;
    }
  }
  return true;
}
bool Move::is_ok_bonanza(const Position & pos)const{

#ifdef LEARN
  return true;
#endif
#ifdef USI
  return true;
#endif
#ifdef MAKE_BOOK
  return true;
#endif
#ifdef SERVER
  return true;
#endif
  if(is_drop()){
    return true;
  }
  const auto prom = is_prom();
  const auto from = this->from();
  const auto to   = this->to();
  const auto me   = pos.turn();
  const auto from_prom = get_prom_area(me).is_seted(from);
  const auto to_prom = get_prom_area(me).is_seted(to);
  const auto pt = piece_to_piece_type(pos.square(from));
  if(!prom && (from_prom || to_prom)){
    if(pt == PAWN || pt == ROOK || pt == BISHOP){
      return false;
    }
    if(pt == LANCE){
        auto r = square_to_rank(to);
      if(pos.turn() == BLACK){
        if(r == RANK_2){
          return false;
        }
      }else{
        if(r == RANK_8){
          return false;
        }
      }
    }
  }
  return true;
}
Move csa_to_move(const std::string s,const Position & pos){

  if(s[1] == '0' && s[2] == '0'){
    File f = static_cast<File>(boost::lexical_cast<int>(s[3]));
    Rank r = static_cast<Rank>(boost::lexical_cast<int>(s[4]));
    f--; r--;
    std::string p = s.substr(5,2);
    HandPiece hp = piece_type_to_hand_piece(csa_piece_to_piece_type(p));
    return make_drop_move(make_square(f,r),hp);
  }else{
    File ff = static_cast<File>(boost::lexical_cast<int>(s[1]));
    Rank rf = static_cast<Rank>(boost::lexical_cast<int>(s[2]));
    File ft = static_cast<File>(boost::lexical_cast<int>(s[3]));
    Rank rt = static_cast<Rank>(boost::lexical_cast<int>(s[4]));
    ff--,rf--,ft--,rt--;
    std::string p = s.substr(5,2);
    PieceType pt = csa_piece_to_piece_type(p);
    if(piece_to_piece_type(pos.square(make_square(ff,rf))) != pt){
      return make_prom_move(make_square(ff,rf),make_square(ft,rt));
    }else{
      return make_move(make_square(ff,rf),make_square(ft,rt));
    }
  }
}
std::string Move::str()const{

  Square from = this->from();
  Square to   = this->to();
  if(!from && !to){
    return "NONE";
  }
  else if(value_ == move_pass().value()){
    return "PASS";
  }
  std::string ret;
  if(this->is_drop()){
    Rank to_rank   = square_to_rank(to);
    File to_file   = square_to_file(to);
    HandPiece p = (HandPiece)(from - SQUARE_SIZE);
    std::stringstream tmp;
    tmp<<(to_file+1)<<(to_rank+1);
    ret = "00" + tmp.str() + hand_piece_str(p) ;
  }else{
    Rank from_rank = square_to_rank(from);
    File from_file = square_to_file(from);
    Rank to_rank   = square_to_rank(to);
    File to_file   = square_to_file(to);
    std::stringstream tmp;
    tmp<<(from_file+1)<<(from_rank+1)<<(to_file+1)<<(to_rank+1);
    ret = tmp.str();
    if(this->is_prom()){
      ret = ret + "!";
    }
  }
  return ret;
}

std::string Move::str(const Position & pos)const{

  Square from = this->from();
  Square to   = this->to();
  if(!from && !to){
    return "NONE";
  }
  else if(value_ == move_pass().value()){
    return "PASS";
  }
  std::string ret;
  if(this->is_drop()){
    Rank to_rank   = square_to_rank(to);
    File to_file   = square_to_file(to);
    HandPiece p = (HandPiece)(from - SQUARE_SIZE);
    std::stringstream tmp;
    tmp<<(to_file+1)<<(to_rank+1);
    ret = "00" + tmp.str() + hand_piece_str(p) ;
  }else{
    Rank from_rank = square_to_rank(from);
    File from_file = square_to_file(from);
    Rank to_rank   = square_to_rank(to);
    File to_file   = square_to_file(to);
    Piece p        = pos.square(from);
    std::stringstream tmp;
    tmp<<(from_file+1)<<(from_rank+1)<<(to_file+1)<<(to_rank+1);
    if(this->is_prom()){
      p = prom_piece(p);
    }
    ret = tmp.str()+str_piece_name(p);
    if(this->is_prom()){
      ret = ret + "!";
    }
  }
  return ret;
}
bool Move::set_sfen(const std::string s){

 if(s == "NONE"){
    value_ = 0;
    return true;
  }else if(s == "PASS"){
    value_ = move_pass().value();
    return true;
  }
  if(s.length() < 4){
    return false;
  }
  const std::string sff = s.substr(0,1);
  const std::string sfr = s.substr(1,1);
  const std::string stf = s.substr(2,1);
  const std::string str = s.substr(3,1);
  if(sfr == "*"){//drop
    File tf;
    Rank tr;
    const auto p = sfen_to_piece(sff);
#ifndef PEFECT_RECORD
   if(!piece_is_ok(p)){
      return false;
    }
#endif
    HandPiece hp = piece_to_hand_piece(p);
    tf = sfen_file_to_file(stf);
    tr = sfen_rank_to_rank(str);
#ifndef PEFECT_RECORD
    if(tf == FILE_SIZE || tr == RANK_SIZE){
      return false;
    }
#endif
    const auto t = make_square(tf,tr);
    set(t,hp);
  }else{
    File ff,tf;
    Rank fr,tr;
    ff = sfen_file_to_file(sff);
    fr = sfen_rank_to_rank(sfr);
    tf = sfen_file_to_file(stf);
    tr = sfen_rank_to_rank(str);
#ifndef PEFECT_RECORD
    if(ff == FILE_SIZE || fr == RANK_SIZE){
      return false;
    }
    if(tf == FILE_SIZE || tr == RANK_SIZE){
      return false;
    }
#endif
    auto f = make_square(static_cast<File>(ff),static_cast<Rank>(fr));
    auto r = make_square(static_cast<File>(tf),static_cast<Rank>(tr));
    if(s.length() == 4){
      set(f,r);
    }else if(s.length() == 5){
      set(f,r,true);
    }else{
      return false;
    }
  }
  return true;
}
std::string Move::str_sfen()const{

  Square from = this->from();
  Square to   = this->to();
  if(!from && !to){
    return "NONE";
  }
  else if(value_ == move_pass().value()){
    return "PASS";
  }
  std::string ret;
  if(this->is_drop()){
    Rank to_rank   = square_to_rank(to);
    File to_file   = square_to_file(to);
    HandPiece hp = (HandPiece)(from - SQUARE_SIZE);
    Piece p = hand_piece_to_piece(hp,BLACK);
    ret = piece_to_sfen(p)
          + "*"
          + file_to_sfen_file(to_file)
          + rank_to_sfen_rank(to_rank);
  }else{
    Rank from_rank = square_to_rank(from);
    File from_file = square_to_file(from);
    Rank to_rank   = square_to_rank(to);
    File to_file   = square_to_file(to);
    ret =   file_to_sfen_file(from_file)
          + rank_to_sfen_rank(from_rank)
          + file_to_sfen_file(to_file)
          + rank_to_sfen_rank(to_rank);
    if(this->is_prom()){
      ret = ret + "+";
    }
  }
  return ret;
}
std::string Move::str_csa(const Position & pos)const{

  // Square from = this->from();
  // Square to   = this->to();
  // if(!from && !to){
  //   return "NONE";
  // }
  // else if(value_ == move_pass().value()){
  //   return "PASS";
  // }
  // std::string ret;
  // if(this->is_drop()){
  //   Rank to_rank   = square_to_rank(to);
  //   File to_file   = square_to_file(to);
  //   HandPiece hp = (HandPiece)(from - SQUARE_SIZE);
  //   auto pt = hand_piece_to_piece_type(hp);
  //   ret = "00"
  //         + file_to_sfen_file(to_file)
  //         + rank_to_sfen_rank(to_rank)
  //         + str_csa_piece_name(pt);
  // }else{
  //   Rank from_rank = square_to_rank(from);
  //   File from_file = square_to_file(from);
  //   Rank to_rank   = square_to_rank(to);
  //   File to_file   = square_to_file(to);
  //   ret =   file_to_sfen_file(from_file)
  //         + rank_to_sfen_rank(from_rank)
  //         + file_to_sfen_file(to_file)
  //         + rank_to_sfen_rank(to_rank);
  //   const auto p = pos.square(from);
  //   auto pt = piece_to_piece_type(p);
  //   if(this->is_prom()){
  //     pt = prom_piece_type(pt);
  //   }
  //   ret += str_csa_piece_name(pt);
  // }
  // return ret;
  Square from = this->from();
  Square to   = this->to();
  if(!from && !to){
    return "%TORYO";
  }
  else if(value_ == move_pass().value()){
    return "%TORYO";
  }
  std::string ret;
  if(this->is_drop()){
    Rank to_rank   = square_to_rank(to);
    File to_file   = square_to_file(to);
    HandPiece hp = (HandPiece)(from - SQUARE_SIZE);
    const auto pt = hand_piece_to_piece_type(hp);
    std::stringstream tmp;
    tmp<<(to_file+1)<<(to_rank+1);
    ret = "00" + tmp.str() + str_csa_piece_name(pt) ;
  }else{
    Rank from_rank = square_to_rank(from);
    File from_file = square_to_file(from);
    Rank to_rank   = square_to_rank(to);
    File to_file   = square_to_file(to);
    Piece p        = pos.square(from);
    auto pt        = piece_to_piece_type(p);
    std::stringstream tmp;
    tmp<<(from_file+1)<<(from_rank+1)<<(to_file+1)<<(to_rank+1);
    if(this->is_prom()){
      pt = prom_piece_type(pt);
    }
    ret = tmp.str()+str_csa_piece_name(pt);
  }
  if(pos.turn() == BLACK){
    ret = "+" + ret;
  }else{
    ret = "-" + ret;
  }
  return ret;
}
