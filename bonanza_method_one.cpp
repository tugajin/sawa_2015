#include "myboost.h"
#include "type.h"
#include "bonanza_method.h"
#include "move.h"
#include "generator.h"
#include "search.h"
#include "futility.h"
#include "random.h"
#include "good_move.h"
#include <vector>
#include <iostream>
#include <string>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>

static volatile u64 record_num;

u32 order(const Position & pos,const Move & record, MoveVal moves[], const int size);

u32 PhaseOne::worker(){

  record_num = 0;
  std::stringstream name;
  name<<"save_pv.pv";
  name<<thread_id_;
  write_pv_stream.open(name.str());
  if(write_pv_stream.fail()){
    std::cout<<thread_id_<<"fail\n";
    return 0;
  }
  while(true){
    //std::cout<<"s\n";
    u32 ret = read_game();
    //std::cout<<"e\n";
    if(ret == 0){
      break;
    }
    else if(ret == 1){
      make_pv();
    }
    record_num++;
    if(record_num %100 == 0){
      std::cout<<".";
    }
    if(record_num % 1000 == 0){
      std::cout<<record_num<<std::endl;
    }
  }
  write_pv_stream.close();
  return 1;
}
u32 PhaseOne::read_game(){

#ifdef TLP
  //lock to read.sfen
  boost::mutex::scoped_lock lk(g_read_lock);
#endif
  std::string buf;
    getline(g_read_record_stream,buf);
#ifdef TLP
  lk.unlock();
#endif
  pos_.init_position();
  record_move_.clear();
  if(buf.empty()){
    return 0;
  }
  size_t loc = buf.find("moves");
  std::string pos_str;
  if(loc == std::string::npos){//おそらくないと思う
    //std::cout<<"not found moves\n";
    goto error;
  }
  if(buf.find("startpos") != std::string::npos){
    //std::cout<<"startpos\n";
    pos_.init_hirate();
    std::vector<std::string> split_buf;
    std::string move_str = buf.substr(loc+5);
    boost::algorithm::split(split_buf,move_str,boost::algorithm::is_space());
    BOOST_FOREACH(std::string & v, split_buf){
      if(v.length() == 4 || v.length() == 5){
        //std::cout<<v<<std::endl;
        Move move(0);
        if(move.set_sfen(v)){
          //std::cout<<move.str()<<std::endl;
          {
            if(!move.is_ok(pos_)){
              goto error;
            }
            if(!pos_.is_ok()){
              goto error;
            }
            if(!pos_.move_is_pseudo(move)){
              goto error;
            }
            CheckInfo ci(pos_);
            if(!pos_.pseudo_is_legal(move,ci.pinned())){
              goto error;
            }
            //std::cout<<move.str(pos_)<<std::endl;
            bool check = pos_.move_is_check(move,ci);
            pos_.do_move(move,ci,check);
            const Bitboard chk = pos_.checker_bb();
            const Value mat = pos_.material();
            pos_.dec_ply();
            pos_.set_checker_bb(chk,0);
            pos_.set_material(mat);
          }
          record_move_.push_back(move);
        }else{
          std::cout<<"fail move\n";
          goto error;
        }
      }
    }
    //BREAK_POINT;
  }else{
    //std::cout<<buf_<<std::endl;
    pos_str = buf.substr(4,loc);
    //std::cout<<"pos:"<<pos_str<<std::endl;
    if(pos_.init_position(pos_str)){
      //pos_.print();
      std::vector<std::string> split_buf;
      std::string move_str = buf.substr(loc+5);
      boost::algorithm::split(split_buf,move_str,boost::algorithm::is_space());
      BOOST_FOREACH(std::string & v, split_buf){
        if(v.length() == 4 || v.length() == 5){
          //std::cout<<v<<std::endl;
          Move move(0);
          if(move.set_sfen(v)){
            //std::cout<<move.str()<<std::endl;
            {
              if(!move.is_ok(pos_)){
                goto error;
              }
              if(!pos_.is_ok()){
                goto error;
              }
              if(!pos_.move_is_pseudo(move)){
                goto error;
              }
              CheckInfo ci(pos_);
              if(!pos_.pseudo_is_legal(move,ci.pinned())){
                goto error;
              }
              //std::cout<<move.str(pos_)<<std::endl;
              bool check = pos_.move_is_check(move,ci);
              pos_.do_move(move,ci,check);
              const Bitboard chk = pos_.checker_bb();
              const Value mat = pos_.material();
              pos_.dec_ply();
              pos_.set_checker_bb(chk,0);
              pos_.set_material(mat);
            }
            record_move_.push_back(move);
          }else{
            goto error;
          }
        }
      }
      //BREAK_POINT;
    }else{
      std::cout<<"fail\n";
      goto error;
    }
  }
  if(buf.find("startpos") != std::string::npos){
    pos_.init_hirate();
  }else{
    ASSERT(pos_str.size() > 1);
    pos_.init_position(pos_str);
  }
  return 1;
error:
  //pos_.init_position();
  return -1;
}
void set_range(const int i,const Value r, Value & a, Value & b){

  if(i == 0){
    a = -VALUE_EVAL_MAX;
    b =  VALUE_EVAL_MAX;
  }else{
    a = r - FV_WINDOW;
    b = r + FV_WINDOW;
    if(a < -VALUE_EVAL_MAX){
      a = -VALUE_EVAL_MAX;
    }
    if(b > VALUE_EVAL_MAX){
      b = VALUE_EVAL_MAX;
    }
  }
}
Depth get_new_depth(const bool check){

  Depth ret = LEARN_DEPTH + (DEPTH_ONE / 2);
  if(check){
    ret += DEPTH_ONE;
  }
  return ret;
}
u32 PhaseOne::make_pv(){

  BOOST_FOREACH(Move & m, record_move_){
    MoveVal * last;
    MoveVal moves[MAX_MOVE_NUM];
    CheckInfo ci(pos_);
    Value record_value,value,root_alpha,root_beta;
    record_value = -VALUE_INF;
    last = moves;
    last = gen_move<LEGAL_MOVE>(pos_,last);
    pv_num_ = 0;
    if(int(last - moves) <= 1){
      ;
    }else if(order(pos_,m,moves,int(last-moves))){

      Value max_value = -VALUE_INF;
      for(int i = 0; i < int(last-moves); i++){
        const bool check = pos_.move_is_check(moves[i].move,ci);
        const auto new_depth = get_new_depth(check);
        set_range(i,record_value,root_alpha,root_beta);
        pos_.init_pv();
        pos_.init_node();
        history.init();
        counter.init();
        follow_up.init();
        gain.init();
        pos_.do_move(moves[i].move,ci,check);

        value = -full_search<PV_NODE>(pos_,-root_beta,-root_alpha,new_depth,false);

        pos_.undo_move(moves[i].move);

        if(i == 0){
          record_value = value;
        }else{
          if(value < root_alpha || value > root_beta){
            continue;
          }
        }
        pos_.update_pv(moves[i].move);
        int j;
        for(j = 0; pos_.pv(0,j).value(); ++j){
          pv_[pv_num_][j] = pos_.pv(0,j);
        }
        pv_[pv_num_][j] = move_none();
        pv_num_++;
        nodes_ += pos_.node_num();
        max_value = std::max(record_value,value);
      }
      if(pv_num_>= 2 && max_value != VALUE_EVAL_MAX){
        write_pv();
        if(record_value == max_value){
          record_move_best_num_++;
        }else{
          other_move_best_num_++;
        }
      }
    }
    bool check = pos_.move_is_check(m,ci);
    pos_.do_move(m,ci,check);
    const Bitboard chk = pos_.checker_bb();
    const Value mat = pos_.material();
    pos_.dec_ply();
    pos_.set_checker_bb(chk,0);
    pos_.set_material(mat);
  }
  //念のため
  record_move_.clear();
  return 1;
}
u32 PhaseOne::write_pv(){

//ファイルの構造
//1行にすべての情報を書き込む
//1〜81 盤上の駒
//手番
//先手の持ち駒　32bitにpackしたもの
//後手の持ち駒  32bitにpackしたもの
//PV　一番初めの手はプロの手 PVの末端には0が入る

  SQUARE_FOREACH(sq){
    write_pv_stream<<pos_.square(sq)<<" ";
  }
  write_pv_stream<<pos_.turn()<<" ";
  write_pv_stream<<pos_.hand(BLACK).value()<<" ";
  write_pv_stream<<pos_.hand(WHITE).value()<<" ";
  for(int i = 0; i < pv_num_; i++){
    for(int j = 0; pv_[i][j].value(); j++){
      write_pv_stream<<(pv_[i][j].value())<<" ";
    }
    write_pv_stream<<0<<" ";
  }
  write_pv_stream<<"\n";
  return 0;
}
void PhaseOne::print(){

  std::cout<<"Phase one info\n ";
  std::cout<<"node : "<<nodes_
           <<"record 1st : "<<record_move_best_num_
           <<"other 1st : "<<other_move_best_num_<<std::endl;
}
void PhaseOne::init_info(const int num){

  thread_id_ = num;
  nodes_ = 0;
  record_move_best_num_ = 0;
  other_move_best_num_ = 0;
}
u32 order(const Position & pos,const Move & record, MoveVal moves[], const int size){

  for(int i = 0; i < size; i++){
    if(record == moves[i].move){
      moves[i].move = moves[0].move;
      moves[0].move = record;
      return 1;
    }
  }
  //bonanzaみたいに一部の不成を生成していないので
  //見つからない時がある
  for(int i = 0; i < size; i++){
    if(record.is_drop()){
      continue;
    }
    const auto rf = record.from();
    const auto rt = record.to();
    const auto mf = moves[i].move.from();
    const auto mt = moves[i].move.to();
    const auto pt = piece_to_piece_type(pos.square(rf));
    const auto rp = record.is_prom();
    const auto mp = moves[i].move.is_prom();
    if(rf == mf && rt == mt && !rp && mp
    && (pt == ROOK || pt == BISHOP || pt == LANCE)){
      moves[i].move = moves[0].move;
      moves[0].move = record;
      return 1;
    }
  }
  std::cout<<"not found order\n";
  return 0;
}
