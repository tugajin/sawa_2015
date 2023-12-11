#include "myboost.h"
#include "sort.h"
#include "position.h"
#include "generator.h"
#include "trans.h"
#include "search_info.h"
#include "search.h"
#include "futility.h"
#include "good_move.h"
#include <limits.h>

static constexpr int debug_flag = 0;
boost::mutex io_lock;
#define DOUT(...) if(debug_flag){\
                                  boost::mutex::scoped_lock lk(io_lock);\
                                  printf("col:%d ",pos_.turn());\
                                  printf("node:%llu ply:%d ",pos_.node_num(),pos_.ply());\
                                  printf("key:%llx ",pos_.key());\
                                  printf(__VA_ARGS__); }
#define DOUT2(...) if(debug_flag){ printf(__VA_ARGS__); }
#define OUT(...) printf(__VA_ARGS__)

bool ok(MoveVal * begin_move, MoveVal * end_move){
  int size = static_cast<int>(end_move - begin_move);
  if(size < 0 || size > 600){
    return false;
  }
  for(int i = 0; i < size; i++){
    if(!begin_move[i].move.is_ok()){
      std::cout<<begin_move[i].move.str()<<std::endl;
      return false;
    }
  }
  return true;
}
void my_sort(MoveVal * begin_move, MoveVal * end_move){
  ASSERT(static_cast<int>(end_move - begin_move) >= 0);
  ASSERT(static_cast<int>(end_move - begin_move) < MAX_MOVE_NUM);
  if(static_cast<int>(end_move - begin_move) < 2){
    return;
  }
  MoveVal tmp_move;
  MoveVal *mp, *mq;
  end_move->value = Value(INT_MIN);//番兵
  for(mp = end_move - 2; mp >= begin_move; --mp){
    tmp_move = *mp;
    for(mq = mp; (mq + 1)->value > tmp_move.value; ++mq){
      *mq = *(mq + 1);
    }
    *mq = tmp_move;
  }
}
inline bool comp_mv(const MoveVal & lhs, const MoveVal & rhs){
  return lhs.value < rhs.value;
}
inline MoveVal *select_best(MoveVal * cur, MoveVal * last){
  std::swap(*cur,*std::max_element(cur,last,comp_mv));
  return cur;
}
inline bool has_positive_value(const MoveVal & move) {
  return move.value > VALUE_ZERO;
}
template<Phase phase>
void note_value(const Position & pos, MoveVal * begin, MoveVal * end){

  for(MoveVal * p = begin; p != end; ++p){
    ASSERT(p->move.is_ok(pos));
    if(false){
    }else if(phase == GOOD_CAPTURE_PHASE){
      p->value = pos.mvv_lva(p->move);
    }else if(phase == QUIET_PHASE1){
      p->value = Value(history.get(p->move.from(),p->move.to()));
    }else if(phase == EVASION_PHASE){
      const auto value = pos.see(p->move,Value(INT_MIN),Value(INT_MAX));
      if(value < 0){
        p->value = value;
      }else if(pos.square(p->move.to())){
        ASSERT(!p->move.is_drop());
        p->value = pos.mvv_lva(p->move);
      }else{
        p->value = value;
      }
    }else{
      ASSERT(false);
      p->value = VALUE_ZERO;
    }
  }
}
//normal search
Sort::Sort(const Position & p, const Depth depth)
  : pos_(p), trans_move_(p.trans_move()), depth_(depth){

  ASSERT(p.is_ok());
  bad_last_ = &moves_[MAX_MOVE_NUM - 1];
  cur_ = last_ = moves_;
  move_num_ = 0;
    killer_[0] = killer_[1] = move_none();
    killer_[2] = killer_[3] = move_none();
    killer_[4] = killer_[5] = move_none();
  if(p.in_checked()){
    DOUT("evasion\n");
    phase_ = EVASION_START;
  }else{
    DOUT("normal\n");
    phase_ = NORMAL_START;
    killer_[0] = p.killer_[p.ply()].move_[0];
    killer_[1] = p.killer_[p.ply()].move_[1];
    if(p.ply() > 0){
      const auto pre  = p.cur_move(p.ply()-1);
      if(pre.value() && pre != move_pass()){
        //counter move
        counter.get(pre.from(),pre.to(),killer_[2],killer_[3]);
        if(killer_[0] == killer_[2] || killer_[1] == killer_[2]){
          killer_[2] = move_none();
        }
        if(killer_[0] == killer_[3] || killer_[1] == killer_[3]){
          killer_[3] = move_none();
        }
      }
    }
    if(p.ply() > 1){
      const auto pre2 = p.cur_move(p.ply()-2);
      if(pre2.value() && pre2 != move_pass()){
        //follow_up move
        follow_up.get(pre2.from(),pre2.to(),killer_[4],killer_[5]);
        if(killer_[0] == killer_[4] || killer_[1] == killer_[4]
            || killer_[2] == killer_[4] || killer_[3] == killer_[4]){
          killer_[4] = move_none();
        }
        if(killer_[0] == killer_[5] || killer_[1] == killer_[5]
            || killer_[2] == killer_[5] || killer_[3] == killer_[5]){
          killer_[5] = move_none();
        }
      }
    }
    if(trans_move_.value()){
      ASSERT(pos_.move_is_pseudo(trans_move_));
      ++last_;
      move_num_++;
    }
  }
}
//quies search
Sort::Sort(const Position & p,
           const int quies_ply) : pos_(p)
                                , trans_move_(p.trans_move())
                                , ply_(quies_ply){

  ASSERT(p.is_ok());
  bad_last_ = &moves_[MAX_MOVE_NUM - 1];
  cur_ = last_ = moves_;
  if(p.in_checked()){
    DOUT("q evasion\n");
    phase_ = EVASION_START;
  }else{
    DOUT("q normal\n");
    phase_ = QUIES_START;
    if(p.ply() > 0){
      recapture_sq_ = p.cur_move(p.ply() - 1).to();
    }else{
      recapture_sq_ = SQUARE_SIZE;
    }
    if(trans_move_.value() && p.move_is_tactical(trans_move_)){
      ASSERT(pos_.move_is_pseudo(trans_move_));
      ++last_;
      move_num_++;
    }
  }
}
//prob cut
Sort::Sort(const Position & p, const Piece piece, const Depth depth)
  :pos_(p), depth_(depth){

  ASSERT(p.is_ok());
  bad_last_ = &moves_[MAX_MOVE_NUM - 1];
  cur_ = last_ = moves_;
  phase_ = PROB_START;
  trans_move_ = p.trans_move();
  if(trans_move_.value() && p.move_is_tactical(trans_move_)){
    last_++;
    move_num_++;
  }else{
    trans_move_ = move_none();
  }
  const auto pt = piece_to_piece_type(piece);
  threshold_ = piece_type_value_ex(pt);
}
template<Color c>Move Sort::next_move(){
  ASSERT(lock_flag_ );
  // if(phase_ == NORMAL_END || phase_ == PROB_END || phase_ == EVASION_END || phase_ == QUIES_END){
  //   return move_none();
  // }
  ASSERT(phase_ != NORMAL_END);
  ASSERT(phase_ != PROB_END);
  ASSERT(phase_ != EVASION_END);
  ASSERT(phase_ != QUIES_END);
  ASSERT(pos_.is_ok());
  ASSERT(cur_ >= moves_);
  ASSERT(cur_ <= moves_ + MAX_MOVE_NUM);
  ASSERT(last_< moves_+MAX_MOVE_NUM);
  ASSERT(c == pos_.turn());
  Move move;
  while(true){
    DOUT("next move start phase = %d num = %d\n",phase_,int(last_ - cur_));
    ASSERT(phase_ == BAD_CAPTURE_PHASE || ok(cur_,last_));
   while(cur_ == last_){
      DOUT("now phase phase = %d\n",phase_);
      next_phase<c>();
      DOUT("update phase phase = %d num = %d\n",phase_,int(last_ - cur_));
    }
    DOUT("passed phase phase = %d num = %d\n",phase_,int(last_ - cur_));
    switch(phase_){
      case NORMAL_START: case PROB_START: case QUIES_START:
        ++cur_;
        ASSERT(trans_move_.value());
        return trans_move_;
      case GOOD_CAPTURE_PHASE:
        move = select_best(cur_,last_)->move;
        ++cur_;
        if(trans_move_ != move &&
           pos_.see(move,Value(INT_MIN),Value(INT_MAX)) >= 0){
          return move;
        }
        DOUT("bad capture %s mvv = %d see = %d\n",move.str().c_str()
                                   ,pos_.mvv_lva(move)
                                   ,pos_.see(move,Value(0),Value(1)));
        bad_last_->move = move;
        --bad_last_;
        break;
      case BAD_CAPTURE_PHASE:
        move = cur_->move;
        --cur_;
        if(move != trans_move_){
          return move;
        }
        break;
      case QUIET_PHASE1:
      case QUIET_PHASE2:
        move = cur_->move;
        ++cur_;
        if( move != trans_move_
         && move != killer_[0]
         && move != killer_[1]
         && move != killer_[2]
         && move != killer_[3]
         && move != killer_[4]
         && move != killer_[5]){
           DOUT("quit move %s\n",move.str().c_str());
          return move;
        }
        break;
      case KILLER_PHASE:
        move = cur_->move;
        ++cur_;
        if(move != trans_move_
        && pos_.move_is_pseudo(move)
        && !pos_.move_is_tactical(move)){
          DOUT("killer move %s\n",move.str().c_str());
          return move;
        }
        DOUT("killer move passed %s\n",move.str().c_str());
        break;
      case EVASION_PHASE:
        move = select_best(cur_,last_)->move;
        ++cur_;
        return move;
      case QUIES_CAPTURE_PHASE:
        move = cur_->move;
        ++cur_;
        if(ply_ >= 5){
          ASSERT(recapture_sq_ != SQUARE_SIZE);
          if(move.to() != recapture_sq_){
            break;
          }
        }
        return move;
        break;
      case PROB_PHASE:
        move = cur_->move;
        ++cur_;
        if(move != trans_move_
        && pos_.see(move,Value(INT_MIN),Value(INT_MAX)) >= threshold_){
          return move;
        }
        break;
      case NORMAL_END:
      case EVASION_END:
      case QUIES_END:
      case PROB_END:
        DOUT("end\n");
        return move_none();
      default:
        ASSERT(false);
    }
  }
  ASSERT(false);
  return move_none();
}
template<Color c>void Sort::next_phase(){

  ASSERT(pos_.is_ok());
  ASSERT(c == pos_.turn());
  DOUT("next phase %d -> %d\n",phase_,phase_+1 );

  switch(++phase_){
    case GOOD_CAPTURE_PHASE:
    case PROB_PHASE:
      cur_ = last_ =  moves_;
      last_ = gen_move<c,CAPTURE_MOVE>(pos_,moves_);
      move_num_ = int(last_ - moves_);
      note_value<GOOD_CAPTURE_PHASE>(pos_,moves_,last_);
      DOUT("gen cap %d \n",int(last_ - moves_));
      return;
    case QUIES_CAPTURE_PHASE:
      cur_ = last_ =  moves_;
      last_ = gen_move<c,CAPTURE_MOVE>(pos_,moves_);
      move_num_ = int(last_ - moves_);
      note_value<GOOD_CAPTURE_PHASE>(pos_,moves_,last_);
      DOUT("gen cap %d \n",int(last_ - moves_));
      return;
    case KILLER_PHASE:
      {
        cur_ = last_ = moves_;
        for(int i = 0; i < 6; ++i){
          if(killer_[i].value()){
            (last_++)->move = killer_[i];
          }
        }
        move_num_ = int(last_ - moves_);
        DOUT("gen killer %d \n",int(last_ - moves_));
      }
      return;
    case QUIET_PHASE1:
      cur_ = last_ = quiet_last_ =  moves_;
      quiet_last_ = gen_move<c,QUIET_MOVE>(pos_,moves_);
      quiet_last_ = gen_move<c,DROP_MOVE>(pos_,quiet_last_);
      move_num_ = int(quiet_last_ - moves_);
      note_value<QUIET_PHASE1>(pos_,moves_,quiet_last_);
      last_ = quiet_last_;
      last_ = std::partition(cur_, quiet_last_, has_positive_value);
      my_sort(moves_,last_);
      DOUT("gen quit %d \n",int(last_ - moves_));
      return;
    case QUIET_PHASE2:
      cur_ = last_;
      last_ = quiet_last_;
      if(depth_ >= 3 * DEPTH_ONE){
        my_sort(cur_,last_);
      }
      break;
    case BAD_CAPTURE_PHASE:
      cur_ = moves_ + MAX_MOVE_NUM - 1;
      last_ = bad_last_;
      move_num_ = int(cur_ - last_);
      return;
    case EVASION_PHASE:
      cur_ = last_ = moves_;
      last_ = gen_move<c,EVASION_MOVE>(pos_,moves_);
      move_num_ = int(last_ - moves_);
      note_value<EVASION_PHASE>(pos_,moves_,last_);
      DOUT("gen evasion %d\n",int(last_ - moves_));
      return;
    case NORMAL_END:
    case EVASION_END:
    case QUIES_END:
    case PROB_END:
      last_ = cur_ + 1;
      DOUT("gen end %d\n",phase_);
      return;
    default:
#ifndef DEBUG
      printf("phase_ %d\n",phase_);
      ASSERT(false);
#endif
      return;
  }
}
Move Sort::next_move(){
  return pos_.turn() ? next_move<WHITE>() : next_move<BLACK>();
}
template Move Sort::next_move<BLACK>();
template Move Sort::next_move<WHITE>();
template void Sort::next_phase<BLACK>();
template void Sort::next_phase<WHITE>();

