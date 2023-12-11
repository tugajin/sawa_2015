#ifndef BONANZA_METHOD_H
#define BONANZA_METHOD_H

#include "type.h"
#include "piece.h"
#include "position.h"
#include "evaluator.h"
#include <vector>
#include <cmath>
#include <iostream>
#include <fstream>
#include <boost/thread/thread.hpp>

class Move;

extern std::ifstream g_read_record_stream;//read record.sfen
extern std::ofstream g_write_of_stream;//output of

extern boost::mutex g_read_lock;

extern int g_record_num;

struct Grad{
  double piece_[PIECE_SIZE];
  float  kkp_[SQUARE_SIZE][SQUARE_SIZE][kkp_end];
  float  pc_on_sq_[SQUARE_SIZE][fe_end][fe_end];
};

class PhaseOne{
  public:
    u32 worker();
    void init_info(const int num);
    u64 nodes_;//ノード数
    void print();
  private:
    static const int RECORD_MAX_LEN = 1024;
    u32 make_pv();
    u32 read_game();
    u32 write_pv();
    void top_record_move();
    Position pos_;
    std::vector<Move> record_move_;
    Move pv_[MAX_MOVE_NUM][MAX_PLY];//bonanza_method用
    int pv_num_;
    u64 record_num_;//棋譜の数
    u64 move_num_;//合法手の数
    u64 move_counted_num_;//学習の対象とした数
    u64 record_move_best_num_;//record の手が一番良かった数
    u64 other_move_best_num_;//その他の手が一番良かった数
    int thread_id_;
    std::ofstream write_pv_stream;//write pv
};
class PhaseTwo{
  public:
    u32 worker();
    void init_info(const int num);
    void print();
    void init();
    u32 update_weight();
    double target_;
    u64 sum_fv_;
    u64 not_zero_num_;
    u64 position_num_;
    Kvalue max_value_;
    Kvalue min_value_;
    Grad grad_;
  private:
    u32 calc_grad();
    void inc_grad(const double dinc);
    u32 read_pv();
    u32 make_list(EvalIndex list0[52],
        EvalIndex list1[52],
        const float f,
        Value & v);
    double piece_grad_[PIECE_TYPE_SIZE];
    int piece_feat_[PIECE_TYPE_SIZE];
    Position pos_;
    Move pv_[MAX_MOVE_NUM][MAX_PLY];
    int pv_num_;
    int thread_id_;
    Piece ar_[SQUARE_SIZE];
    std::ifstream read_pv_stream;//read pv of
};
class BonanzaMethod{
  public:
    enum{ TLP_NUM = 1 };
    //でかすぎて失敗するので動的に確保
    BonanzaMethod(){
      // try{
        phase_one_ = new PhaseOne[TLP_NUM];
        phase_two_ = new PhaseTwo[TLP_NUM];
      // }
      // catch(...){
      //   std::cout<<"Bonanza Method new error\n";
      //   exit(1);
      // }
      g_write_of_stream.open("of.csv");
    }
    ~BonanzaMethod(){
      delete [] phase_one_;
      delete [] phase_two_;
      g_write_of_stream.close();
    }
    PhaseOne * phase_one_;
    PhaseTwo * phase_two_;
    Value sum_material_;
    void init();
    u32 write_weight();
    u32 phase_one(const u64 iterate);
    u32 phase_two(const u64 iterate);
    void merge_phase_two();
};
extern void learn();

static const Depth LEARN_DEPTH = Depth(1 * int(DEPTH_ONE));
static const Value FV_WINDOW = Value(256);
static const double FV_PENALTY = 0.2 / (double)FV_SCALE;

inline double func(Value x){

  const double delta = (double)FV_WINDOW / 7.0;
  double d;
  if(x < -FV_WINDOW){
    x = -FV_WINDOW;
  }else if(x > FV_WINDOW){
    x = FV_WINDOW;
  }
  d = 1.0 / (1.0 + exp(double(-x)/(double)delta));
  return d;
}
inline double dfunc(const double x){

  const double delta = (double)FV_WINDOW / 7.0;
  double dd, dn, dtemp, dret;
  if(x <= -FV_WINDOW){
    dret = 0.0;
  }else if(x >= FV_WINDOW){
    dret = 0.0;
  }else{
    dn = exp(-x / delta);
    dtemp = dn + 1.0;
    dd = delta * dtemp * dtemp;
    dret = dn / dd;
  }
  return dret;
}
inline Value sign(const double g){

  if(g > 0.0){
    return Value(1);
  }else if(g < 0.0){
    return Value(-1);
  }else{
    return Value(0);
  }
}

#endif
