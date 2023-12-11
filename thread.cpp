#include "myboost.h"
#include "thread.h"
#include "search.h"
#include <iostream>

#ifdef TLP

Thread g_thread;


void start_address(const int tid);
void wait_work(const int tid, Position * parent);
void copy_position(Position * parent, Position * child, const Value value);
Position * find_child();

constexpr int CPUS = 3;

void Thread::init(){

  max_num_ = CPUS;
  stop_    = 0;
  num_     = 0;
  idle_    = 0;
  work_[0].tlp_id_ = 0;
  work_[0].tlp_stop_ = false;
  work_[0].tlp_used_ = true;
  work_[0].tlp_slot_ = 0;
  work_[0].tlp_sibling_num_ = 0;
  for(int i = 0; i < max_num_; i++){
    work_->tlp_sibling_[i] = nullptr;
  }
  for(int i = 1; i < TLP_NUM_WORK; i++){
    work_[i].tlp_slot_ = i;
    work_[i].tlp_used_ = false;
  }
}
bool tlp_start(){

  if(g_thread.max_num_ <= 1){
    return true;
  }
  // if(g_thread.max_num_ != CPUS){
  //   printf("CPUS error %d\n",g_thread.max_num_);
  //   exit(1);
  // }
  if(g_thread.num_){
    return true;
  }
  for(int num = 1; num < g_thread.max_num_; num++){
    //スレッドを作る
    g_thread.th_[num] = new boost::thread(start_address,num);
  }
  while(g_thread.num_ + 1 < g_thread.max_num_){
    ;
  }
  return true;
}
void tlp_end(){

  g_thread.stop_ = true;
  while(g_thread.num_){
    ;
  }
  g_thread.stop_ = false;
}
void start_address(const int tid){

  g_thread.ptr_[tid] = nullptr;

  boost::mutex::scoped_lock lk(g_thread.lock_);
  std::cout<<"Hi from thread no."<<tid<<std::endl;
  g_thread.sum_real_timer_[tid] = 0;
  g_thread.cpu_timer_[tid].start();
  g_thread.block_[tid] = 0;
  g_thread.num_++;
  g_thread.idle_++;
  lk.unlock();

  wait_work(tid,nullptr);

  lk.lock();
  std::cout<<"Bye from thread no."<<tid<<std::endl;
  g_thread.num_--;
  g_thread.idle_--;
  lk.unlock();
  g_thread.stop_ = false;
}
void wait_work(const int tid, Position * parent){

  Position * slot;
  Value value;
  for(;;){
    for(;;){
      //仕事を割り当てられた
      if(g_thread.ptr_[tid] != nullptr){
        break;
      }
      //探索のメインスレッドで、並列に探索しているものが終わったら
      if(parent != nullptr && !parent->tlp_sibling_num_){
        break;
      }
      //すべての探索終了
      if(g_thread.stop_){
        return;
      }
    }
    boost::mutex::scoped_lock lk(g_thread.lock_);
    if(g_thread.ptr_[tid] == nullptr){
      g_thread.ptr_[tid] = parent;
    }
    g_thread.idle_--;
    lk.unlock();
    slot = g_thread.ptr_[tid];
    //メインスレッドなのでおしまい
    if(slot == parent){
      return;
    }
    g_thread.real_timer_[tid].start();
    g_thread.block_[tid]++;
    if(slot->tlp_parent_->tlp_node_type_ == PV_NODE){
      value = tlp_full_search<PV_NODE>(*slot
                                       ,slot->tlp_parent_->tlp_alpha_
                                       ,slot->tlp_parent_->tlp_beta_
                                       ,slot->tlp_parent_->tlp_depth_
                                       ,slot->tlp_parent_->tlp_cut_node_);
    }
    else{
      ASSERT(slot->tlp_parent_->tlp_node_type_ == NOPV_NODE);
      value = tlp_full_search<NOPV_NODE>(*slot
                                         ,slot->tlp_parent_->tlp_alpha_
                                         ,slot->tlp_parent_->tlp_beta_
                                         ,slot->tlp_parent_->tlp_depth_
                                         ,slot->tlp_parent_->tlp_cut_node_);
    }
    lk.lock();
    copy_position(slot->tlp_parent_,slot,value);
    slot->tlp_parent_->tlp_sibling_num_--;
    slot->tlp_parent_->tlp_sibling_[tid] = nullptr;
    slot->tlp_used_ = false;
    g_thread.ptr_[tid] = nullptr;
    g_thread.idle_++;
    g_thread.block_[tid]--;
    g_thread.sum_real_timer_ [tid]+= g_thread.real_timer_[tid].elapsed();
    lk.unlock();
  }
}
//探索した結果を親の局面に伝える
void copy_position(Position * parent, Position * child, const Value value){

  ASSERT(parent != nullptr);
  ASSERT(child != nullptr);
  if(!child->node_num()){
    return;
  }
  parent->inc_node(child->node_num());
  boost::mutex::scoped_lock lk(parent->tlp_lock);
  //beta cutが起きてないまたは探索が途中でストップしたらコピーしない
  if(child->tlp_stop_ || value <= parent->tlp_best_value_){
    lk.unlock();
    return;
  }
  ASSERT(parent->tlp_best_value_ < value);
  ASSERT(value_is_ok(value));
  parent->tlp_best_value_ = value;
  const int ply = parent->ply();
  const Move cur = child->cur_move(ply);
  parent->set_cur_move(ply,cur);
  //parent->update_pv(child->tlp_best_move_,child);
  for(int i = ply; i < ply + 10; ++i){
    for(int j = ply; j < ply + 10; ++j){
      parent->set_pv(child->pv(i,j),i,j);
    }
  }
  lk.unlock();
}
//この局面をストップ。この局面の子供についてもストップさせている
//時間切れかbeta cutが起きた場合に呼ばれる
void tlp_set_stop(Position * pos){

  pos->tlp_stop_ = true;
  for(int num = 0; num < g_thread.max_num_; num++){
    if(pos->tlp_sibling_[num] != nullptr){
      tlp_set_stop(pos->tlp_sibling_[num]);
    }
  }
}
//alphabeta探索の途中で並列探索したいときの個々に来る。
//（1手以上探索した後で、スレッドが余っている場合）
bool tlp_split(Position * pos){

  Position * child;
  boost::mutex::scoped_lock lk(g_thread.lock_);
  if(!g_thread.idle_ || pos->tlp_stop_){
    lk.unlock();
    return false;
  }
  g_thread.ptr_[pos->tlp_id_] = nullptr;
  pos->tlp_sibling_num_ = 0;
  int child_num = 0;
  for(int num = 0; num < g_thread.max_num_; num++){
    //使用中のスレッドは使っちゃダメ
    if(g_thread.ptr_[num] != nullptr){
      pos->tlp_sibling_[num] = nullptr;
    }else{
      child = find_child();
      if(child == nullptr){
        continue;
      }
      child_num++;
      for(int i = 0; i < g_thread.max_num_; i++){
        child->tlp_sibling_[i] = nullptr;
      }
      child->tlp_parent_ = pos;
      child->tlp_id_ = num;
      child->tlp_used_ = true;
      child->tlp_stop_ = false;
      pos->tlp_sibling_[num] = child;
      pos->tlp_sibling_num_++;
      child->init_position(*pos);
      g_thread.ptr_[num] = child;//これをした瞬間仕事を待っているスレッドが動き出す
                                 //メインスレッドもここで動かしてしまうみたい
    }
  }
  //分割出来なかったらこのスレッドが担当
  if(!child_num){
    g_thread.ptr_[pos->tlp_id_] = pos;
    lk.unlock();
    return false;
  }
  g_thread.split_++;
  g_thread.idle_++;
  lk.unlock();
  //ほかのスレッドが終わるのを待つ
  //printf("main thread waiting %d\n",pos->tlp_id_);
  wait_work(pos->tlp_id_,pos);
  return true;
}
Position * find_child(){

  int i;
  for(i = 1; i < TLP_NUM_WORK && g_thread.work_[i].tlp_used_; i++){
    ;
  }
  if(i == TLP_NUM_WORK){
    return nullptr;
  }
  if(i > g_thread.slot_num_){
    g_thread.slot_num_ = i;
  }
  return g_thread.work_ + i;
}
void Thread::print() {
  std::cout<<"\n";
  std::cout<<"info string ";
  std::cout<<"slot "<<slot_num_<<" ";
  std::cout<<"split "<<split_<<" ";
  std::cout<<"stop "<<stop_num_<<" \n";
   for(int i = 1; i < max_num_; i++){
     int sum = sum_real_timer_[i] + real_timer_[i].elapsed();
     int all = cpu_timer_[i].elapsed();
     if(all){
       double time = double(sum) / double(all);
       std::cout<<"cpu ("<<i<<") : "<< time<<std::endl;
     }
   }
}
#endif
