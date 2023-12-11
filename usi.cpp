#include "myboost.h"
#include "init.h"
#include "usi.h"
#include "root.h"
#include "check_info.h"
#include "futility.h"
#include "trans.h"
#include "random.h"
#include "book.h"
#include "test.h"
#include "thread.h"
#include "search.h"
#include <vector>
#include <string>
#include <unistd.h>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>

int Usi::loop(){

  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stdin, NULL, _IONBF, 0);
  fflush(stdin);
  fflush(stdout);

  begin_receive_thread();
  while(true){
    std::string command = get_command();
    if(command.find("usinewgame") != std::string::npos){
    }else if(command.find("isready") != std::string::npos){
      receive_stop_flag_ = false;
      root.tmg_.stop_ = false;
      init_game();
      g_thread.work_[0].init_hirate();
      printf("readyok\n");
    }else if(command.find("usi") != std::string::npos){
      printf("id name sawa_5\n");
      printf("id author Takuya Ugajin\n");
      printf("usiok\n");
    }else if(command.find("setoption name USI_Ponder value true") != std::string::npos){
      do_ponder_ = true;
    }else if(command.find("setoption name USI_Ponder value false") != std::string::npos){
      do_ponder_ = false;
    }else if(command.find("position") != std::string::npos){//局面をセット
      set_position(command);
    }else if(command.find("go") != std::string::npos){//探索開始
      //持ち時間をbufから得る
      set_time(command);
      root.init();
      root.set_time_limit(g_thread.work_[0]);
      //思考開始
      root.iterate(g_thread.work_[0]);
      if(receive_stop_flag_){
      }else{
        print_best_move();
      }
      search_info.print();
      //g_thread.print();
    } else if(command.find("stop") != std::string::npos){
      receive_stop_flag_ = false;
      print_best_move();
    }else if(command.find("gameover") != std::string::npos){
      //exit_game();
    }else if(command == "display"){
      g_thread.work_[0].print();
    }else if(command == "test"){
      test();
    }else if(command == "q"){
      break;
    }else {
      std::cout<<"error command "<<command<<std::endl;
    }
  }
  exit_game();
  return 0;
}
void Usi::receive_message(){

  std::string buf;
  while(true){
    getline(std::cin,buf);
    if(buf.find("quit") != std::string::npos){
      while(command_queue_.size() > 0){
        usleep(100000);
      }
      exit(0);
    }
    //クリティカルセクションに入る。
    boost::mutex::scoped_lock lk(receive_lock_);
    if(buf.find("stop") != std::string::npos){
      //stopが来たら直ちに探索を終了する
      root.tmg_.stop_ = true;
      receive_stop_flag_ = true;
    }
    if(buf.find("gameover") != std::string::npos){
      receive_stop_flag_ = true;
      root.tmg_.stop_ = true;
    }
    std::string command_str = buf;
    command_queue_.push(command_str);//コマンドをキューに追加。
    lk.unlock();//クリティカルセッションを出る。
  }
}
std::string Usi::get_command(){

  while(command_queue_.empty()){
    usleep(100000);
  }
  boost::mutex::scoped_lock lk(receive_lock_);//クリティカルセッションに入る
  std::string ret = command_queue_.front();//キューからコマンドを取得
  command_queue_.pop();//キューからコマンドを削除
  lk.unlock();//クリティカルセッションを出る。
  return ret;
}
void Usi::begin_receive_thread(){
    th_ = new boost::thread(&Usi::receive_message,this);
}
void Usi::set_time(const std::string buf){

  int ibt,iwt,iby;
  //適当
  ibt = iwt = 1000;
  iby = 0;
  //持ち時間が設定されていない場合大きな値を与える
  if(buf.find("infinite") != std::string::npos){
    root.tmg_.black_remain_time_= INT_MAX / 3;
    root.tmg_.white_remain_time_ = INT_MAX / 3;
    root.tmg_.byoyomi_ = INT_MAX / 3;
    return;
  }
  std::vector<std::string> split_buf;
  boost::algorithm::split(split_buf,buf,boost::algorithm::is_space());
  for(u32 i = 0; i < split_buf.size(); i++){
    if(split_buf[i].find("btime") != std::string::npos){
      ibt = boost::lexical_cast<int>(split_buf[i+1]);
    }else if(split_buf[i].find("wtime") != std::string::npos){
      iwt = boost::lexical_cast<int>(split_buf[i+1]);
    }else if(split_buf[i].find("byoyomi") != std::string::npos){
      iby = boost::lexical_cast<int>(split_buf[i+1]);
    }
  }
  std::cout<<boost::format("bt %d wt %d byo %d\n") % ibt % iwt % iby;
  root.tmg_.black_remain_time_ = ibt;
  root.tmg_.white_remain_time_ = iwt;
  root.tmg_.byoyomi_ = iby;
}
void Usi::set_position(const std::string buf){

  std::cout<<buf<<std::endl;
  const auto index = buf.find("position sfen ");
  if(index != std::string::npos){
    const auto pos_buf = buf.substr(index+14);
    g_thread.work_[0].init_position(pos_buf);
  }else{
    g_thread.work_[0].init_hirate();
  }
  const auto index2 = buf.find("moves ");
  if(index2 == std::string::npos){
    return;
  }
  const auto move_buf = buf.substr(index2 + 6);
  std::vector<std::string> split_buf;
  boost::algorithm::split(split_buf,move_buf,boost::algorithm::is_space());
  BOOST_FOREACH(const auto &m, split_buf){
    Move move;
    move.set_sfen(m);
    CheckInfo ci(g_thread.work_[0]);
    g_thread.work_[0].do_move(move,ci,g_thread.work_[0].move_is_check(move,ci));
    const auto chk = g_thread.work_[0].checker_bb();
    const auto mat = g_thread.work_[0].material();
    const auto k = g_thread.work_[0].key();
    g_thread.work_[0].dec_ply();
    g_thread.work_[0].set_checker_bb(chk,0);
    g_thread.work_[0].set_material(mat);
    g_thread.work_[0].set_key(k);
  }
}
void Usi::print_best_move() const {
  //最善手を返す
  if(root.best_move_ == move_none()){
    printf("bestmove resign\n");
  }else{
    printf("bestmove %s",root.best_move_.str_sfen().c_str());
    printf("\n");
  }
}

