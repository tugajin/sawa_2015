#include "myboost.h"
#include "test.h"
#include "init.h"
#include "position.h"
#include "root.h"
#include "thread.h"
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <boost/algorithm/string.hpp>

void string_to_answer(const std::string &str, std::vector<Move> &ans);

void test(){

  std::ifstream ifs("problem.txt");

  if(ifs.fail()){
    std::cout<<"can't open material.txt\n";
    exit(1);
  }
  std::string str;
  while(getline(ifs,str)){
    std::cout<<str<<std::endl;
    init_game();
    g_thread.work_[0].init_position(str);
    g_thread.work_[0].print();
    root.init();
    root.set_time_limit(20000,20000,20000);
    root.iterate(g_thread.work_[0]);
#ifdef TLP
    tlp_end();
#endif
    root.best_move_.print();
    std::vector<Move> answer;
    string_to_answer(str,answer);
    auto itr = std::find(answer.begin(),answer.end(),root.best_move_);
    if(answer.size() == 0){
      std::cout<<"ERROR NOT FOUND ANS\n";
    }else if(itr == answer.end()){
      std::cout<<"INCORRECT\n";
    }else{
      std::cout<<"CORRECT\n";
    }
  }
}
void string_to_answer(const std::string &str, std::vector<Move> &ans){
  ans.clear();
  const auto loc = str.find("moves ");
  if(loc == std::string::npos){
    return;
  }
  auto move_str = str.substr(loc);
  std::vector<std::string> move_str_list;
  boost::algorithm::split(move_str_list,move_str,boost::algorithm::is_space());
  move_str_list.erase(move_str_list.begin());
  std::cout<<"answer\n";
  for(auto &ms : move_str_list){
    Move move;
    move.set_sfen(ms);
    ans.push_back(move);
    std::cout<<move.str()<<std::endl;
  }
}
