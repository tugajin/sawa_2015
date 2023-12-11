#include "myboost.h"
#include "root.h"
#include "init.h"
#include "search.h"
#include "generator.h"
#include "trans.h"
#include "futility.h"
#include "book.h"
#include "thread.h"
#include <algorithm>
#include <functional>
#include <boost/format.hpp>
#include <boost/foreach.hpp>

Root root;

void Root::iterate(Position & pos){

  init_turn(pos);

  gen_root_move(pos);

  if(root_moves_.empty()){
    std::cout<<"resign\n";
    best_move_ = move_none();
    return;
  }
  if(pos.rep_ply_ < 30){

    Move book_move = book.move(pos);
    std::cout<<"book move is "<<book_move.str()<<std::endl;
    BOOST_FOREACH(auto & x, root_moves_){
      if(x.move_ == book_move){
        best_move_ = book_move;
        value_list_[0] = VALUE_ZERO;
        pos.set_pv(book_move, 0,0);
        return;
      }
    }
  }
#ifdef TLP
  tlp_start();
#endif

  auto root_alpha = -VALUE_INF;
  auto root_beta  = VALUE_INF;

  std::cout<<"start searching...\n";

  for(auto iterate_ply = 1; iterate_ply < MAX_PLY - 10; ++iterate_ply){

    start_iterate(iterate_ply);
    set_window(root_alpha, root_beta);

    while(true){

      value_list_[iterate_ply]
        = search(pos, root_alpha,root_beta, DEPTH_ONE * iterate_ply);
      if(stop()){
        break;
      }
      else if(value_list_[iterate_ply] >= root_beta){
        root_beta = reset_fail_high_window(root_beta);
      }else if(value_list_[iterate_ply] <= root_alpha){
        root_alpha = reset_fail_low_window(root_alpha);
      }else if(std::abs(value_list_[iterate_ply]) > VALUE_EVAL_MAX){
        tmg_.stop_ = true;
        break;
      }else{
        break;
      }
    }
    if(stop()){
      break;
    }
    tmg_.check_difficult(value_list_, searching_ply_);
    auto elapsed = tmg_.timer_.elapsed();
    if(tmg_.is_easy_ && searching_ply_ >= 4 && !tmg_.is_difficult_ && elapsed >= tmg_.time_limit_0_ / 4){
      break;
    }
  }
  best_move_ = pos.pv(0,0);
}
Value Root::search(Position & pos, Value alpha, Value beta, const Depth depth){

  if(root.is_time_up()){
    printf("root time up\n");
    return VALUE_ZERO;
  }

  int size = root_moves_.size();
  auto old_alpha = alpha;
  auto value = VALUE_NONE;
  auto best_value = VALUE_NONE;
  CheckInfo ci(pos);

  for(int i = 0; i < size; ++i){

    auto move = root_moves_[i].move_;
    auto check = pos.move_is_check(move,ci);

    pos.do_move(move, ci, check);

    Depth new_depth = depth - DEPTH_ONE;
    Depth ext = DEPTH_NONE;
    Depth reduce = DEPTH_NONE;

    if(check){
      ext = DEPTH_ONE;
    }
    if(i > 2
    && !check
    && !pos.in_checked()
    && !pos.move_is_tactical(move)){
      reduce = DEPTH_NONE;
    }

    new_depth = new_depth + ext - reduce;

    if(i == 0){
      value = -full_search<PV_NODE>(pos, -beta, -alpha, new_depth, false);
    }else{
      value = -full_search<NOPV_NODE>(pos, -alpha-1, -alpha, new_depth, false);
      if(value > alpha && reduce){
        new_depth += reduce;
        value = -full_search<NOPV_NODE>(pos, -alpha-1, -alpha, new_depth, false);
        if(value > alpha){
          value = -full_search<PV_NODE>(pos, -beta, -alpha, new_depth, false);
        }
      }
    }
    pos.undo_move(move);
    if(stop()){
      return best_value;
    }

    root_moves_[i].pre_value_ = root_moves_[i].value_;

    if(value <= alpha){
      root_moves_[i].value_ = old_alpha;
    }else if(value >= beta){
      root_moves_[i].value_ = beta;
    }else{
      root_moves_[i].value_ = value;
    }
    if(value > best_value){
      best_value = value;
      if(value > alpha){
        alpha = value;
        opp_best_move_ = pos.pv(0,1);
        pos.update_pv(move);
        if(searching_ply_ > 1){
          print_pv(pos, best_value);
        }
        if(value >= beta){
          return value;
        }
      }
      if(i != 0){
        tmg_.is_easy_ = false;
      }
    }
  }
  std::stable_sort(root_moves_.begin(), root_moves_.end());
  if(best_value > old_alpha && best_value < beta){
    //insert_pv_in_TT(pos,0);
  }
  return best_value;
}
void Root::gen_root_move(Position & pos){
  ASSERT(pos.is_ok());
  std::cout<<"gen_root_move...";
  MoveVal moves[MAX_MOVE_NUM];
  MoveVal * pm = moves;
  RootMoveList rm;
  const auto me = pos.turn();
  CheckInfo ci(pos);

  root_moves_.clear();

  if(pos.in_checked()){
    pm = gen_move<EVASION_MOVE>(pos,pm);
  }else{
    pm = gen_move<CAPTURE_MOVE>(pos,pm);
    pm = gen_move<QUIET_MOVE>(pos,pm);
    pm = gen_move<DROP_MOVE>(pos,pm);
  }
  for(MoveVal * p = moves; p != pm; ++p){
    rm.move_ = p->move;
    rm.is_check_ = pos.move_is_check(p->move,ci);
    rm.pre_value_ = VALUE_ZERO;
    pos.do_move(p->move,ci,rm.is_check_);
    if(pos.check_in_checked(me)){
      pos.undo_move(p->move);
      continue;
    }
    //千日手はここで排除する
    //手番がひっくり返っているのでloseのとき優先する
    int dummy = 0;
    switch(pos.check_rep(0,20,dummy)){
      case HAND_IS_WIN:
        pos.undo_move(p->move);
        continue;
        break;
      case HAND_IS_LOSE:
        break;
      case HAND_IS_SAME:
        pos.undo_move(p->move);
        continue;
        break;
      default:
        break;
    }
    //rm.value_ = VALUE_ZERO;
    rm.value_ = -full_quiescence(pos,-VALUE_INF,VALUE_INF,0);
    pos.undo_move(p->move);
    if(rm.value_ > -VALUE_EVAL_MAX){
      if(p->move.is_prom()){
        //成る手を優先
        rm.value_++;
      }
      root_moves_.push_back(rm);
    }
  }
  if(!root_moves_.empty()){
    std::stable_sort(root_moves_.begin(),root_moves_.end());
    value_list_[0] = root_moves_[0].value_;
    best_move_ = root_moves_[0].move_;
    tmg_.check_easy(root_moves_);
  }
  std::cout<<"end("<<root_moves_.size()<<")\n";
}
void Root::insert_pv_in_TT(Position & pos, const int ply){

  Move move = pos.pv(0,ply);
  if(move == move_none() || !move.is_ok(pos)){
    return;
  }
  auto value = -VALUE_EVAL_MAX;
  auto key = pos.key();
  TT.store(pos,key,DEPTH_QS,move,value,TRANS_NONE);
  CheckInfo ci(pos);
  pos.do_move(move,ci,pos.move_is_check(move,ci));
  insert_pv_in_TT(pos,ply+1);
  pos.undo_move(move);
}
void Root::set_time_limit(const Position & pos){

  constexpr int C = 100;
  constexpr int TIME_MAX_PLY = 120;
  const auto my_remain_time = pos.turn() ? tmg_.white_remain_time_ : tmg_.black_remain_time_;
  const auto my_ply = pos.rep_ply_;
  int l1,l2;
  l1 = l2 = 0;
  if(my_remain_time < 60 * 1000 && tmg_.byoyomi_ == 0){
    //残り時間が１分未満になったら残り１秒指し
    l1 = l2 = 1000;
  }else if(my_remain_time <= 30 * 1000 && tmg_.byoyomi_ != 0){
    //秒読み対応
    l1 = l2 = tmg_.byoyomi_ - 1000;
  }else{
  //「Time Management for Monte-Carlo Tree Search Applied to the Game of Go」
  //の式を引用
    l1 = my_remain_time / (C + std::max(TIME_MAX_PLY - my_ply,0));
    l2 = l1 * 5;
    //1秒未満は切り捨てなので、一律0.8秒に設定
    l1 = ((l1 / 1000) * 1000) + 800;
    l2 = ((l2 / 1000) * 1000) + 800;
    l1 += tmg_.byoyomi_;
    l2 += tmg_.byoyomi_;
    l1 = std::max(l1,1000);
    l2 = std::max(l2,1000);
    printf("my limit is %d %d\n",l1,l2);
  }

  tmg_.time_limit_0_ = l1;
  tmg_.time_limit_1_ = l2;
  tmg_.time_limit_2_ = l2;
}
void Root::set_time_limit(const int l0, const int l1, const int l2){
  tmg_.time_limit_0_ = l0;
  tmg_.time_limit_1_ = l1;
  tmg_.time_limit_2_ = l2;
  printf("my limit is %d %d %d\n",l0,l1,l2);
}
bool Root::is_time_up(){

#ifdef LEARN
  return false;
#else
  if(tmg_.stop_){
    return true;
  }
  if(tmg_.nodes_ < 0){
    tmg_.nodes_ += TimeManager::CHECK_NODES_EPOCH;
    const auto elapsed= tmg_.timer_.elapsed();
    //std::cout<<"elapsed "<<elapsed<<" "<<tmg_.time_limit_0_<<std::endl;
    if(searching_ply_ > 1 && elapsed >= tmg_.time_limit_0_ ){
       //std::cout<<"stop1\n";
      if(elapsed >= tmg_.time_limit_1_){
         auto stable = (elapsed >= tmg_.time_limit_2_) || (!tmg_.fail_low_num_) || (!tmg_.is_difficult_);
         //std::cout<<"stop2\n";
        if(stable){
           //std::cout<<"stop3 "<<elapsed<<" "<< tmg_.fail_low_num_<<" \n";
          tmg_.stop_ = true;
          return true;
        }
      }
    }

  }
  return false;
#endif
}
void TimeManager::check_easy(const std::vector<RootMoveList>
                              & rm){

  if(byoyomi_){
    return;
  }
  if(rm.size() <= 1){
    is_easy_ = true;
    return;
  }
  const Value best_value = rm[0].value_;
  const Value second_value = rm[1].value_;
  const Value base_value = piece_type_value_ex(PROOK);
  if(best_value > second_value + (base_value * 2 / 8)){
    is_easy_= true;
    std::cout<<"root move looks easy!\n";
  }
}
void TimeManager::check_difficult(const std::array<Value, MAX_PLY> &v, const int ply){
  if(byoyomi_){
    return;
  }
  if(ply > 0){
    this->is_difficult_ = (v[ply] - v[ply - 1] > 500);
    if(is_difficult_){
      std::cout<<"difficult!!\n";
    }
  }
}
void Root::print_pv(const Position & pos, const Value value)const{

  printf("info depth %d seldepth %d nodes %llu",searching_ply_, searching_max_ply_, pos.node_num());
  printf(" pv");
  for(int i = 0; pos.pv(0,i).value(); i++){
    const auto move = pos.pv(0,i);
    printf(" %s",move.str_sfen().c_str());
    //printf(" %s",move.str().c_str());
  }
  if(value > VALUE_EVAL_MAX){
    printf(" score cp mate +");
  }
  else if(value < -VALUE_EVAL_MAX){
    printf(" score cp mate -");
  }else{
    printf(" score cp %d",value);
  }
  printf("\n");
}

