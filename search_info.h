#ifndef SEARCH_INFO_H
#define SEARCH_INFO_H

#include "type.h"
#include "thread.h"
#include "trans.h"

class SearchInfo{
  public:
    SearchInfo(){
      init();
    }
    void init(){
      try_mate_distance_ = 0;
      pruned_mate_distance_ = 0;;
      try_null_ = 0;;
      pruned_null_ = 0;;
      ext_ = 0;
      singuler_ext_ = 0;
      half_ext_ = 0;
      reduce_ = 0;
      research_no1_ = 0;
      research_no2_ = 0;
      rep_same_ = 0;
      rep_win_ = 0;
      rep_lose_ = 0;
      rep_short_cut_ = 0;
      try_iid_ = 0;
      try_eval_ = 0;
      try_stored_eval_ = 0;
      try_futility_ = 0;
      pruned_futility_ = 0;
      try_delta_ = 0;
      pruned_delta_ = 0;
      try_mate_ = 0;
      hit_mate_trans_ = 0;
      ok_mate_ = 0;
      try_razoring_ = 0;
      pruned_razoring_ = 0;
      pruned_move_count_ = 0;
      pruned_see_ = 0;
    }
    void print(){
      std::cout<<"info string mate distance "<<try_mate_distance_<<std::endl;
      std::cout<<"info string mate_distance ok "<<pruned_mate_distance_<<std::endl;
      std::cout<<"info string try_null "<<try_null_<<std::endl;
      std::cout<<"info string try_null ok "<<pruned_null_<<std::endl;
      std::cout<<"info string ext "<<ext_<<std::endl;
      std::cout<<"info string ext(s) "<<singuler_ext_<<std::endl;
      std::cout<<"info string ext(h) "<<half_ext_<<std::endl;
      std::cout<<"info string reduce_ "<<reduce_<<std::endl;
      std::cout<<"info string research 1 "<<research_no1_<<std::endl;
      std::cout<<"info string research 2 "<<research_no2_<<std::endl;
      std::cout<<"info string repetion same "<<rep_same_<<std::endl;
      std::cout<<"info string repetion win "<<rep_win_<<std::endl;
      std::cout<<"info string repetion lose "<<rep_lose_<<std::endl;
      std::cout<<"info string repetion short "<<rep_short_cut_<<std::endl;
      std::cout<<"info string iid "<<try_iid_<<std::endl;
      std::cout<<"info string call eval "<<try_eval_<<std::endl;
      std::cout<<"info string call stored eval "<<try_stored_eval_<<std::endl;
      std::cout<<"info string hit stored eval "<<try_stored_eval2_<<std::endl;
      std::cout<<"info string try futility "<<try_futility_<<std::endl;
      std::cout<<"info string try futility ok "<<pruned_futility_<<std::endl;
      std::cout<<"info string try delta "<<try_delta_<<std::endl;
      std::cout<<"info string try delta ok "<<pruned_delta_<<std::endl;
      std::cout<<"info string try mate "<<try_mate_<<std::endl;
      std::cout<<"info string hit mate "<<hit_mate_trans_<<std::endl;
      std::cout<<"info string mate ok "<<ok_mate_<<std::endl;
      std::cout<<"info string try razor "<<try_razoring_<<std::endl;
      std::cout<<"info string try razor ok "<<pruned_razoring_<<std::endl;
      std::cout<<"info string try count ok "<<pruned_move_count_<<std::endl;
      std::cout<<"info string try see ok "<<pruned_see_<<std::endl;
      g_thread.print();
      //TT.print_info();
    }
    u64 try_mate_distance_;
    u64 pruned_mate_distance_;
    u64 try_null_;
    u64 pruned_null_;
    u64 ext_;
    u64 singuler_ext_;
    u64 half_ext_;
    u64 reduce_;
    u64 research_no1_;
    u64 research_no2_;
    u64 rep_same_;
    u64 rep_win_;
    u64 rep_lose_;
    u64 rep_short_cut_;
    u64 try_iid_;
    u64 try_eval_;
    u64 try_stored_eval_;
    u64 try_stored_eval2_;
    u64 try_futility_;
    u64 pruned_futility_;
    u64 try_delta_;
    u64 pruned_delta_;
    u64 try_mate_;
    u64 hit_mate_trans_;
    u64 ok_mate_;
    u64 try_razoring_;
    u64 pruned_razoring_;
    u64 pruned_move_count_;
    u64 pruned_see_;
};

#endif
