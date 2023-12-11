#ifndef TRANS_H
#define TRANS_H

#include "limits.h"
#include "type.h"
#include "hand.h"
#include "move.h"
#include <boost/utility.hpp>

class Position;

enum TransFlag{ TRANS_NONE = 0,TRANS_UPPER = 1, TRANS_LOWER = 2,
                TRANS_EXACT = TRANS_UPPER | TRANS_LOWER, };

struct Entry{
  u32 lock;
  u32 hand_b;
  u16 move;
  s16 value;
  u8 age;
  s8 depth;
  s8 flag;//valueはflag以下かどうか
  s8 pad;
};
class TranspositionTable : boost::noncopyable{
  public:
    TranspositionTable(){
      init();
      init_info();
      table_ = nullptr;
    }
    ~TranspositionTable(){
      trans_free();
    }
    void allocate(u64 target);
    void clear();
    void trans_free();
    void inc_age();
    void print_info()const;
    int age()const{
      return age_;
    }
    void store(const Position & pos, const Key key, Depth depth, Move & move, Value  value, const TransFlag flag);
    TransFlag probe(Position & pos, const Key key, const Depth depth, const Value alpha, const Value beta, Value & value, Depth & trans_depth);
    void init(){
      size_ = 0;
      mask_ = 0;
    }
    void init_info(){
      age_  = 0;
      used_num_ = 0;
      read_num_ = 0;
      read_hit_key_num_ = 0;
      read_hit_hand_num_ = 0;
      read_hit_hand_good_num_ = 0;
      read_hit_hand_bad_num_ = 0;
      upper_num_ = 0;
      lower_num_ = 0;
      exact_num_ = 0;
      write_num_ = 0;
      write_overwrite_num_ = 0;
      write_collision_num_ = 0;
      avoid_null_num_ = 0;
      illegal_move_num_ = 0;
    }
    //private:
    Entry * table_;
    u64 size_;
    u32 mask_;
    int age_;
    u32 used_num_;
    u32 read_num_;
    u32 read_hit_key_num_;
    u32 read_hit_hand_num_;
    u32 read_hit_hand_good_num_;
    u32 read_hit_hand_bad_num_;
    u32 upper_num_;
    u32 lower_num_;
    u32 exact_num_;
    u32 write_num_;
    u32 write_overwrite_num_;
    u32 write_collision_num_;
    u32 avoid_null_num_;
    u32 illegal_move_num_;
    static const int CLUSTER_SIZE = 4;
};

static inline bool trans_flag_is_exact_lower(const TransFlag f){
  ASSERT(TRANS_LOWER & TRANS_LOWER);
  ASSERT(TRANS_EXACT & TRANS_LOWER);
  return f & TRANS_LOWER;
}
static inline bool trans_flag_is_exact_upper(const TransFlag f){
  ASSERT(TRANS_UPPER & TRANS_UPPER);
  ASSERT(TRANS_EXACT & TRANS_UPPER);
  return f & TRANS_UPPER;
}
static inline u32 get_lock(const Key key){
  return static_cast<u32>((key) >> 32);
}

static inline bool entry_is_empty(const Entry & e){
  return (!e.flag);
}

extern TranspositionTable TT;
#endif
