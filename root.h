#ifndef ROOT_H
#define ROOT_H

#include "myboost.h"
#include "type.h"
#include "move.h"
#include "position.h"
#include "check_info.h"
#include "time.h"
#include <vector>
#include <array>
#include "boost/utility.hpp"
//#include <boost/filesystem.hpp>

class RootMoveList{
  public:
    Move move_;
    Value value_;
    Value pre_value_;
    bool is_check_;
    bool operator < (const RootMoveList& rhs)const{
      return this->value_ > rhs.value_;
    }
};
class TimeManager{
  public:
    static const s64 CHECK_NODES_EPOCH = 1024;
    void init(){
      time_limit_0_ = 0;
      time_limit_1_ = 0;
      time_limit_2_ = 0;
      node_limit_ = 0;
      is_easy_ = false;
      is_difficult_ = false;
      stop_ = false;
      timer_.start();
      fail_low_num_ = 0;
      fail_high_num_ = 0;
      nodes_ = CHECK_NODES_EPOCH;
    }
    void check_easy(const std::vector<RootMoveList> & rm);
    void check_difficult(const std::array<Value, MAX_PLY> &v, const int ply);
    volatile s64 nodes_;
    int time_limit_0_;
    int time_limit_1_;
    int time_limit_2_;
    int node_limit_;
    int black_remain_time_;
    int white_remain_time_;
    int byoyomi_;
    int fail_high_num_;
    int fail_low_num_;
    bool is_easy_;
    bool is_difficult_;
    volatile bool stop_;
    Timer timer_;
};
class Root{
  public:
    void init(){
      tmg_.init();
      searching_ply_ = 0;
      searching_max_ply_ = 0;
      best_move_ = move_none();
      opp_best_move_ = move_none();
    }
    Value search( Position & pos, Value alpha, Value beta, const Depth depth);
    void set_time_limit(const Position & pos);
    void set_node_limit(const u64 limit){
      tmg_.node_limit_ = limit;
    }
    void set_time_limit(const int l0, const int l1, const int l2);
    bool is_time_up();
    void dec_node(){
      --tmg_.nodes_;
    }
    bool stop()const{
      return tmg_.stop_;
    }
    Value best_value()const{
      int p = 1;
      if(ply_is_ok(searching_ply_) && p >=1){
        p = searching_ply_;
      }
      return value_list_[p - 1];
    }
    Move best_move_;
    Move opp_best_move_;
    TimeManager tmg_;
    int searching_ply_;
    int searching_max_ply_;
    void iterate(Position & pos);
    std::vector<RootMoveList> root_moves_;
    void gen_root_move(Position & pos);
  private:
    void insert_pv_in_TT(Position & pos, const int ply);
    void start_iterate(const int p){
      tmg_.fail_high_num_ = 0;
      tmg_.fail_low_num_ = 0;
      searching_ply_ = p;
      searching_max_ply_ = 0;
      asp_delta_ = ASP_WINDOW;
    }
    void set_window(Value & a, Value &b){
      const int p = searching_ply_;
      if(p <= 5){
        a = -VALUE_INF;
        b =  VALUE_INF;
      }else{
        a = std::max(value_list_[p - 1] - ASP_WINDOW, -VALUE_INF);
        b = std::min(value_list_[p - 1] + ASP_WINDOW,  VALUE_INF);
      }
      //std::cout<<"window a : "<<a<<" b : "<<b<<std::endl;
    }
    Value reset_fail_high_window(const Value b){
      ++tmg_.fail_high_num_;
      asp_delta_ += asp_delta_ * 2;
      return std::min(b + asp_delta_, VALUE_INF);

    }
    Value reset_fail_low_window(const Value a){
      ++tmg_.fail_low_num_;
      asp_delta_ += asp_delta_ * 2;
      return std::max(a - asp_delta_, -VALUE_INF);
    }
    void print_pv(const Position & pos, const Value value)const;
    std::array<Value, MAX_PLY> value_list_;
    Value asp_delta_;
    static const Value ASP_WINDOW = Value(15);
};

extern Root root;

#endif
