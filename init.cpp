#include "init.h"
#include "bitboard.h"
#include "type.h"
#include "position.h"
#include "check_info.h"
#include "move.h"
#include "hand.h"
#include "piece.h"
#include "misc.h"
#include "generator.h"
#include "search.h"
#include "sort.h"
#include "evaluator.h"
#include "trans.h"
#include "random.h"
#include "search_info.h"
#include "futility.h"
#include "bonanza_method.h"
#include "usi.h"
#include "book.h"
#include "good_move.h"
#include "thread.h"

void init_once(){
  init_bitboard();
  init_relation();
  init_key();
  init_evauator();
  init_eval_trans();
  init_mate_trans();
  init_rand();
  init_futility();
  history.init();
  counter.init();
  follow_up.init();
  gain.init();
  search_info.init();
  TT.allocate(1024);
  TT.init_info();
#ifdef TLP
  g_thread.init();
#endif
}
void init_game(){
    init_eval_trans();
    init_mate_trans();
    search_info.init();
    TT.clear();
    book.open();
    g_thread.init();
}
void init_turn(Position & pos){
  std::cout<<"init turn\n";
  TT.inc_age();
  search_info.init();
  history.init();
  counter.init();
  follow_up.init();
  gain.init();
  pos.init_pv();
  pos.init_node();
  pos.set_eval(STORED_EVAL_IS_EMPTY,0);
  pos.set_eval(STORED_EVAL_IS_EMPTY,1);
  pos.tlp_stop_ = false;
  g_thread.split_ = 0;
  g_thread.stop_num_ = 0;
  g_thread.slot_num_ = 0;
}
void exit_game(){
  TT.trans_free();
  book.close();
  tlp_end();
}
