#ifndef THREAD_H
#define THREAD_H

#include "position.h"
#include "time.h"
#include <boost/thread/thread.hpp>

//max_thread * 8 == num_work
bool tlp_start();
void tlp_end();
void tlp_set_stop(Position * pos);
bool tlp_split(Position * pos);

struct Thread{

  void init();
  void print() ;
  ~Thread(){
    tlp_end(); 
  }
  boost::mutex lock_;
  Position work_[TLP_NUM_WORK];
  Position * volatile ptr_[TLP_MAX_THREAD];
  boost::thread *th_[TLP_MAX_THREAD];
  volatile bool stop_;
  volatile int idle_;
  volatile int num_;
  int max_num_;
  int split_;
  int stop_num_;
  int slot_num_;
  int block_[TLP_MAX_THREAD];
  Timer cpu_timer_[TLP_MAX_THREAD];
  Timer real_timer_[TLP_MAX_THREAD];
  int sum_real_timer_[TLP_MAX_THREAD];

};
extern Thread g_thread;

#endif
