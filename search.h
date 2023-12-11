#ifndef SEARCH_H
#define SEARCH_H

#include "myboost.h"
#include "type.h"
#include "search_info.h"
#include "root.h"

class Position;

template<Color c,NodeType nt> extern Value full_search(Position & pos, Value alpha, Value beta, Depth depth, bool cut_node);
template<Color c> extern Value full_quiescence(Position & pos, Value alpha, Value beta, int quies_ply);
template<Color c,NodeType nt> extern Value tlp_full_search(Position & pos, Value alpha, Value beta, Depth depth, bool cut_node);

template<NodeType nt>static inline Value full_search(Position & pos, Value alpha, Value beta, Depth depth, bool cut_node){
  return pos.turn() ? full_search<WHITE,nt>(pos,alpha,beta,depth,cut_node)
                    : full_search<BLACK,nt>(pos,alpha,beta,depth,cut_node);
}
template<NodeType nt>static inline Value tlp_full_search(Position & pos, Value alpha, Value beta, Depth depth, bool cut_node){
  return pos.turn() ? tlp_full_search<WHITE,nt>(pos,alpha,beta,depth,cut_node)
                    : tlp_full_search<BLACK,nt>(pos,alpha,beta,depth,cut_node);
}
static inline Value full_quiescence(Position & pos, Value alpha, Value beta, int quies_ply){
  return pos.turn() ? full_quiescence<WHITE>(pos,alpha,beta,quies_ply)
                    : full_quiescence<BLACK>(pos,alpha,beta,quies_ply);
}

extern u64 perft(Position & pos,int ply,const int max_ply);

extern SearchInfo search_info;


#endif
