#ifndef PV_H
#define PV_H

#include "type.h"
#include "move.h"

class Pv{
  public:
    Pv(){
      for(int i = 0; i < MAX_PLY - 1; i++){
        init(i);
      }
    }
    void init(const int ply){
      move_[ply][ply] = move_none();
      move_[ply][ply+1] = move_none();
    }
    void update(const Move & move, const int ply){
      move_[ply][ply] = move;
      int i;
      for(i = ply + 1; move_[ply + 1][i].value() ; ++i){
        ASSERT(i < MAX_PLY);
        move_[ply][i] = move_[ply + 1][i];
      }
      move_[ply][i] = move_none();
    }
    void update(const Move & move, const int ply, Pv * pv){
      move_[ply][ply] = move;
      int i;
      for(i = ply + 1; pv->move(ply+1,i).value() ; ++i){
        ASSERT(i < MAX_PLY);
        move_[ply][i] = pv->move(ply+1,i);
      }
      move_[ply][i] = move_none();
    }
    bool is_ok(const int ply)const{
      for(int i = 0; i < MAX_PLY; ++i){
        if(!move_[ply][i].value()){
          return true;
        }
      }
      return false;
    }
    Move move(const int ply, const int ply2)const{
      return move_[ply][ply2];
    }
    void set_move(const Move & m, const int ply, const int ply2){
      move_[ply][ply2] = m;
    }
  private:
    Move move_[MAX_PLY][MAX_PLY];
};
static inline bool pv_cmp(const Pv & a, const Pv & b){

  for(int i = 0; i < MAX_PLY; i++){
    for(int j = 0; j < MAX_PLY; j++){
      if(a.move(i,j) != b.move(i,j)){
        return false;
      }
    }
  }
  return true;
}
#endif
