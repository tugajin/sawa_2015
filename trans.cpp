#include "type.h"
#include "trans.h"
#include "position.h"
#include "search.h"
#include "futility.h"
#include <bitset>

TranspositionTable TT;

void TranspositionTable::allocate(u64 target){

#ifdef LEARN
  return;
#endif

  ASSERT(table_ == nullptr);
  ASSERT(size_==0);

  if(target < 16){
    target = 16;
  }
  target *= 1024 * 1024;//to Mbyte
  u64 size = 1;
  for(size = 1; size != 0 && size <= target; size *= 2){
    ;
  }
  //std::cout<<"size "<<size<<" target "<<target<<std::endl;
  size /= 2;
  size /= sizeof(Entry);
  ASSERT(size != 0);
  ASSERT(static_cast<int>(size&(size-1))==0);
  size_ = size + TranspositionTable::CLUSTER_SIZE - 1;
  mask_ = (size - 1);
  std::cout<<"TranspositionTable("
           <<(size * sizeof(Entry))/(1024*1024)<<"Mb)...";
  table_ = (Entry *) malloc(size_ * sizeof(Entry));
  if(table_ == nullptr){
    std::cout<<"Failed TranspositionTable allocate\n";
    exit(1);
  }
  std::cout<<"end\n";
  clear();
}
void TranspositionTable::clear(){

#ifdef LEARN
  return;
#endif

  ASSERT(table_ != nullptr);
  ASSERT(size_!=0);
  std::cout<<"TranspositionTable cleaning...";
  for(u32 i = 0; i < size_; i++){
    table_[i].lock = 0;
    table_[i].hand_b = 0;
    table_[i].move = 0;
    table_[i].value = 0;
    table_[i].age = 0;
    table_[i].depth = 0;
    table_[i].flag = 0;
    table_[i].pad = 0;
  }
  std::cout<<"end\n";
}
void TranspositionTable::trans_free(){

#ifdef LEARN
  return;
#endif

  if(table_ != nullptr){
    free(table_);
    table_ = nullptr;
  }
  init();
  init_info();
}
void TranspositionTable::inc_age(){

#ifdef LEARN
  return;
#endif

  ASSERT(table_ != nullptr);
  ++age_;
}
void TranspositionTable::store(const Position & pos,
                               const Key key,
                                     Depth depth,
                                     Move & move,
                                     Value  value,
                               const TransFlag flag){
#ifdef LEARN
  return;
#endif

  ASSERT(key != 0);
  ASSERT(table_ != nullptr);
  ASSERT(depth >= DEPTH_QS);

  write_num_++;

  const auto lock = get_lock(key);
  const auto hand_b = pos.hand(BLACK);
  const auto index = key & mask_;
  auto best_value = INT_MIN;
  Entry *best, *pentry;

  best = nullptr;
  pentry = &table_[index];
  if(value > VALUE_EVAL_MAX){
    value -= pos.ply() - 1;
  }else if(value < -VALUE_EVAL_MAX){
    value += pos.ply() - 1;
  }
  for(int i = 0; i < TranspositionTable::CLUSTER_SIZE; ++i,++pentry){
    const auto entry = *pentry;
    //中身がないなら書き込む
    if(entry_is_empty(entry)){
      used_num_++;
      best = pentry;
      break;
    }else if(entry.lock == lock && entry.hand_b == hand_b.value()){
        //同じ局面なら書き換えてしまう
      write_overwrite_num_++;
      best = pentry;
      //書き換えるデータの手が空なら元々の手を採用
      if(!move.value()){
        move = Move(pentry->move);
      }
      break;
    }
    auto value = ((TT.age() != entry.age) ? 1024 : -1024) - entry.depth;
    if(value > best_value){
      best = pentry;
      best_value = value;
    }
  }
  //TODO
  write_collision_num_++;

  best->lock = lock;
  best->hand_b = hand_b.value();
  best->move = move.value();
  best->value = value;
  best->age = TT.age();
  best->depth = depth;
  best->flag = flag;
}
TransFlag TranspositionTable::probe( Position & pos,
                               const Key key,
                               const Depth depth,
                               const Value alpha,
                               const Value beta,
                               Value & value,
                               Depth & trans_depth){
#ifdef LEARN
  return TRANS_NONE;;
#endif

  ASSERT(table_ != nullptr);
  read_num_++;
  const auto index = key & mask_;
  const auto lock = get_lock(key);
  const u32 hand_value = pos.hand(BLACK).value();
  Entry *pentry = &table_[index];

  for(int i = 0; i < TranspositionTable::CLUSTER_SIZE; ++i,++pentry){
    Entry entry = *pentry;
    const auto hash_lock = entry.lock;
    const u32 hash_hand_value = entry.hand_b;
    const Depth hash_depth = static_cast<Depth>(entry.depth);
    trans_depth = hash_depth;
    value = static_cast<Value>(entry.value);
    if(value > VALUE_EVAL_MAX){
      value -= pos.ply() - 1;
    }else if(value < -VALUE_EVAL_MAX){
      value += pos.ply() - 1;
    }
    if(hash_lock == lock){
      read_hit_key_num_++;
      if(hash_hand_value == hand_value){
        read_hit_hand_num_++;
        pos.set_trans_move(Move(entry.move));
        if(pos.trans_move().value() && !pos.move_is_pseudo(pos.trans_move())){
          TT.illegal_move_num_++;
          pos.set_trans_move(move_none());
          continue;
        }
        const TransFlag flag = static_cast<TransFlag>(entry.flag);
        if(flag == TRANS_LOWER
            && beta <= value
            && (hash_depth >= depth
              || std::abs(value) > VALUE_EVAL_MAX)){
          lower_num_++;
          return TRANS_LOWER;
        }
        if(flag == TRANS_UPPER
            && alpha >= value
            && (hash_depth >= depth
              || std::abs(value) > VALUE_EVAL_MAX)){
          upper_num_++;
          return TRANS_UPPER;
        }
        if(flag == TRANS_EXACT
            && (hash_depth >= depth
              || std::abs(value) > VALUE_EVAL_MAX)){
          exact_num_++;
          return TRANS_EXACT;
        }
      }else{//hand
        const Hand hash_hand(hash_hand_value);
        const Hand hand(hand_value);
        value = static_cast<Value>(entry.value);
        const auto flag = static_cast<TransFlag>(entry.flag);
        //hashのほうが悪い
        //下限は確定
        if(((pos.turn() == BLACK) && (hand >= hash_hand))
        || ((pos.turn() == WHITE) && (hand <= hash_hand))){
          pos.set_trans_move(Move(entry.move));
          if(pos.trans_move().value() && !pos.move_is_pseudo(pos.trans_move())){
            pos.set_trans_move(move_none());
            continue;
          }
          if((hash_depth >= depth || abs(value) > VALUE_EVAL_MAX)
              && trans_flag_is_exact_lower(flag)
              && value >= beta){
            read_hit_hand_good_num_++;
            return TRANS_LOWER;
          }
        }
        //今の局面のほうが悪い
        //上限は確定
        else if(((pos.turn() == BLACK) && (hand <= hash_hand))
        || ((pos.turn() == WHITE) && (hand >= hash_hand))){
          pos.set_trans_move(Move(entry.move));
          if(pos.trans_move().value() && !pos.move_is_pseudo(pos.trans_move())){
            pos.set_trans_move(move_none());
            continue;
          }
          if((hash_depth >= depth || abs(value) > VALUE_EVAL_MAX)
              && trans_flag_is_exact_upper(flag)
              && value <= alpha){
            read_hit_hand_good_num_++;
            return TRANS_LOWER;
          }
        }
        //とりあえず合法手ならOKにしてみる
        else{
          pos.set_trans_move(Move(entry.move));
          if(pos.trans_move().value() && !pos.move_is_pseudo(pos.trans_move())){
            pos.set_trans_move(move_none());
            continue;
          }
        }
      }
    }//key
  }//cluster
  return TRANS_NONE;
}
void ::TranspositionTable::print_info()const{

  std::cout<<"info string transposition info\n"
           <<"info string used : "<<used_num_<<std::endl
           <<"info string used(%) : "<<double(double(used_num_) /double(TT.size_))*100<<std::endl
           <<"info string read : "<<read_num_<<std::endl
           <<"info string read key : "<<read_hit_key_num_<<std::endl
           <<"info string read hand : "<<read_hit_hand_num_<<std::endl
           <<"info string read hand good : "<<read_hit_hand_good_num_<<std::endl
           <<"info string read hand bad : "<<read_hit_hand_bad_num_<<std::endl
           <<"info string upper : "<<upper_num_<<std::endl
           <<"info string lower : "<<lower_num_<<std::endl
           <<"info string exact : "<<exact_num_<<std::endl
           <<"info string write : "<<write_num_<<std::endl
           <<"info string write overwrite: "<<write_overwrite_num_<<std::endl
           <<"info string write collision : "<<write_collision_num_<<std::endl
           <<"info string illegal : "<<illegal_move_num_<<std::endl;
}

