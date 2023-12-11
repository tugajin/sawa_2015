#include "piece.h"
#include "type.h"
#include <boost/static_assert.hpp>

const std::string csa_piece_name[] =  {
  "xx","TO","NY","NK","NG","UM","RY","OU","KI",
  "FU","KY","KE","GI","KA","HI","xx",};
const std::string sfen_name[PIECE_SIZE_EX] ={
  "・",
  "xx","xx","xx","xx","xx","xx","K","G","P","L","N","S","B","R",
  "？","？",
  "xx","xx","xx","xx","xx","xx","k","g","p","l","n","s","b","r",
  "？","？",
};
std::string str_piece_name(const Piece p){

  ASSERT(p == EMPTY ||piece_is_ok(p));
  return piece_name[p];
}
std::string str_piece_type_name(const PieceType p){

  ASSERT(p == EMPTY_T || piece_type_is_ok(p));
  //hack PAWN == PAWN_B
  BOOST_STATIC_ASSERT(static_cast<int>(PAWN) == static_cast<int>(PAWN_B));
  return str_piece_name(static_cast<Piece>(p));
}
std::string str_csa_piece_name(const PieceType pt){

  ASSERT(csa_piece_name[PAWN] == "FU");
  ASSERT(csa_piece_name[PPAWN] == "TO");
  ASSERT(piece_type_is_ok(pt));
  return csa_piece_name[pt];
}
PieceType csa_piece_to_piece_type(const std::string s){

  for(int i = 0; i < 16; i++){
    if(s == csa_piece_name[i]){
      return static_cast<PieceType>(i);
    }
  }
  return EMPTY_T;
}
Piece sfen_to_piece(const std::string s){
  //for(Piece p = PPAWN_B; p < PIECE_SIZE;p++){
  //  if(s == sfen_name[p]){
  //    return p;
  //  }
  //}
  //高速化のためif文で展開
  if(false){
  }else if(s == "K"){
    return KING_B;
  }else if(s == "G"){
    return GOLD_B;
  }else if(s == "P"){
    return PAWN_B;
  }else if(s == "L"){
    return LANCE_B;
  }else if(s == "N"){
    return KNIGHT_B;
  }else if(s == "S"){
    return SILVER_B;
  }else if(s == "B"){
    return BISHOP_B;
  }else if(s == "R"){
    return ROOK_B;
  }else if(s == "k"){
    return KING_W;
  }else if(s == "g"){
    return GOLD_W;
  }else if(s == "p"){
    return PAWN_W;
  }else if(s == "l"){
    return LANCE_W;
  }else if(s == "n"){
    return KNIGHT_W;
  }else if(s == "s"){
    return SILVER_W;
  }else if(s == "b"){
    return BISHOP_W;
  }else if(s == "r"){
    return ROOK_W;
  }
  return EMPTY;
}
std::string piece_to_sfen(const Piece p){

  return sfen_name[p];
}
