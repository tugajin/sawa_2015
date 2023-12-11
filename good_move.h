#ifndef GOODMOVE_H
#define GOODMOVE_H

#include "type.h"
#include "move.h"
#include <utility>
#include <cstring>

template<bool is_gain, class T>class GoodMove{
  public:
    static const int GOOD_MAX = 2000;
    void init(){
      std::memset(table_,0,sizeof(table_));
    }
    //update history
    void update(const Square from, const Square to, const int v){
      if(abs(table_[std::min(from,SQUARE_SIZE)][to]) < GOOD_MAX)
        table_[std::min(from,SQUARE_SIZE)][to] += v;
    }
    //update gain
    void update(const Square from, const Square to, const Value v){
      table_[std::min(from,SQUARE_SIZE)][to] = std::max(table_[from][to]-1,v);
    }
    //update counter
    void update(const Square from, const Square to, const Move & m){
      if(table_[std::min(from,SQUARE_SIZE)][to].first == m){
        return;
      }
      table_[std::min(from,SQUARE_SIZE)][to].second
        = table_[std::min(from,SQUARE_SIZE)][to].first;
      table_[std::min(from,SQUARE_SIZE)][to].first = m;
    }
    T get(const Square from, const Square to){
      return table_[std::min(from,SQUARE_SIZE)][to];
    }
    void get(const Square from, const Square to, Move & m1, Move & m2){
      m1 = table_[std::min(from,SQUARE_SIZE)][to].first;
      m2 = table_[std::min(from,SQUARE_SIZE)][to].second;
    }
  private:
    static const int INDEX1 = int(SQUARE_SIZE)+1;
    static const int INDEX2 = int(SQUARE_SIZE);
    T table_[INDEX1][INDEX2];
};
typedef std::pair<Move, Move> Counter;
typedef GoodMove<false,int>HistoryMove;
typedef GoodMove<false,Counter>CounterMove;
typedef GoodMove<true,Value>Gain;

extern HistoryMove history;
extern CounterMove counter;
extern CounterMove follow_up;
extern Gain gain;

#endif
