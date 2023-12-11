#include "myboost.h"
#include "type.h"
#include "move.h"
#include "position.h"
#include "generator.h"
#include "search.h"
#include "check_info.h"
#include "evaluator.h"
#include "sort.h"
#include "trans.h"
#include "futility.h"
#include "root.h"
#include "good_move.h"
#include "thread.h"
#include <bitset>

static constexpr int debug_flag = 0;
boost::mutex io_lock2;
#define DOUT(...) if(debug_flag){ \
                                  boost::mutex::scoped_lock lk(io_lock2);\
                                  printf("key = %llu ",pos.key());\
                                  printf("node = %llu ",pos.node_num());\
                                  printf("ply = %d ",pos.ply());\
                                  printf("id = %d ",pos.tlp_id_);\
                                  printf("stop = %d ",root.tmg_.stop_);\
                                  printf(__VA_ARGS__); }
#define OUT(...) if(true){\
                                  printf("key = %llu ",pos.key());\
                                  printf("node = %llu ",pos.node_num());\
                                  printf("ply = %d ",pos.ply());\
                                  printf("id = %d ",pos.tlp_id_);\
                                  printf("stop = %d ",root.tmg_.stop_);\
                                  printf(__VA_ARGS__); }

static constexpr bool USE_RAZORING      = true;
static constexpr bool USE_SINGULAR      = true;
static constexpr bool USE_SEE           = true;
static constexpr bool USE_FUTILITY      = true;
static constexpr bool USE_COUNT         = true;
static constexpr bool USE_DELTA         = true;
static constexpr bool USE_PROB_CUT      = true;
static constexpr bool USE_EVASION       = true;
static constexpr bool USE_DISTANCE      = true;
static constexpr bool USE_MATE          = true;
static constexpr bool USE_TRANS         = true;
static constexpr bool USE_NULL          = true;
static constexpr bool USE_IID           = true;
static constexpr bool USE_HALF          = true;
static constexpr bool USE_HALF_RESEARCH = true;


SearchInfo search_info;

static inline Depth get_null_depth(const Depth depth){
  return depth - (3 * DEPTH_ONE + depth / 4);
}
static inline Depth get_iid_depth(const Depth depth){
  return depth - (2 * DEPTH_ONE);
}
static inline Value razor_margin(const Depth d){
  return Value(512 + 16 * int(d));
}

static inline Depth limit_extension(const Depth ext, const int ply){
  auto ret_ext = ext;
  if(ext && ply > 2 * root.searching_ply_){
    if(ply < 4 * root.searching_ply_){
      ret_ext *= 4 * root.searching_ply_ - ext;
      ret_ext /= 2 * root.searching_ply_;
    }else{ ret_ext = DEPTH_NONE; }
  }
  return ret_ext;
}

template<Color c, NodeType nt>
Value full_search(Position & pos, Value alpha, Value beta, Depth depth, bool cut_node){

  ASSERT(pos.is_ok());
  ASSERT(pos.ply() > 0);
  ASSERT(value_is_ok(alpha));
  ASSERT(value_is_ok(beta));
  ASSERT(alpha < beta);
  ASSERT(c == pos.turn());
  ASSERT(nt == PV_NODE || ((nt == NOPV_NODE) && (alpha + 1 == beta)));
  //printf("[normal id:%d idle:%d]",pos.tlp_id_,g_thread.idle_);
  DOUT("start search a = %d b = %d depth = %d eval_org%d\n",alpha,beta,depth,pos.eval());
  pos.init_pv();
  pos.inc_node();
  root.dec_node();
  pos.set_skip_move(pos.ply()+1,move_none());
  pos.set_cur_move(pos.ply(),move_none());
  pos.set_eval(STORED_EVAL_IS_EMPTY,pos.ply()+1);
  root.searching_max_ply_ = std::max(root.searching_max_ply_,pos.ply());

  if(depth < DEPTH_ONE ){
    Value value = full_quiescence<c>(pos,alpha,beta,0);
    //Value value = evaluate(pos);
    DOUT("quies in %d\n",value);
    return value;
  }
  Value old_alpha = alpha;
  //mate distance pruning
  if(USE_DISTANCE){
    search_info.try_mate_distance_++;
    alpha = std::max( value_mate(pos.ply()  ), alpha);
    beta  = std::min(-value_mate(pos.ply()-1), beta );
    if(alpha >= beta){
      search_info.pruned_mate_distance_++;
      DOUT("mate distance %d\n",alpha);
      return alpha;
    }
  }

  //rep
  int dummy = 0;
  switch(pos.check_rep(0,6,dummy)){
    case HAND_IS_WIN:
      {
        search_info.rep_win_++;
        Value value = VALUE_REP_WIN;
        return value;
      }
    case HAND_IS_LOSE:
      {
        search_info.rep_lose_++;
        Value value = -VALUE_REP_WIN;
        return value;
      }
    case HAND_IS_SAME:
      {
        search_info.rep_same_++;
        //pos.ply() & 1 -> root_turn != pos.turn()
        //お互い千日手はマイナス無限ということにしてるので、下のように書かないとだめ
        Value value = (pos.ply() & 1) ? -VALUE_REP : VALUE_REP;
        return value;
      }
    default: break;
  }
  if((!pos.tlp_id_ && root.is_time_up()) || pos.tlp_stop_){
    return VALUE_ZERO;
  }
  DOUT("skip rep eval_org%d\n",pos.eval());
  Depth trans_depth = DEPTH_NONE;
  Value trans_value = VALUE_ZERO;
  Value value       = VALUE_NONE;

  Move skip_move(0);
  skip_move = pos.skip_move(pos.ply());

  pos.set_trans_move(move_none());

  TransFlag trans_flag = TRANS_NONE;

  const auto key = (!skip_move.value()) ? pos.key() : pos.singular_key();

  if(USE_TRANS){

    trans_flag = TT.probe(pos,key,depth,alpha,beta,trans_value,trans_depth);

    switch(trans_flag){
      case TRANS_UPPER:
        if(nt == NOPV_NODE){
          DOUT("upper cut\n");
          return trans_value;
        }
        break;
      case TRANS_LOWER:
        if(nt == NOPV_NODE){
          DOUT("lower cut\n");
          return trans_value;
        }
        break;
      case TRANS_EXACT:
        DOUT("exact cut\n");
        return trans_value;
        break;
      default:
        break;
    }
  }
  if(pos.ply() >= MAX_PLY-10){
    printf("near max ply\n");
    return evaluate<c>(pos);
  }
  CheckInfo ci(pos);
  Move move;
  DOUT("mae eval_org%d\n",pos.eval());
  Value static_eval = evaluate<c>(pos);
  DOUT("ato eval_org%d\n",pos.eval());
  if(pos.in_checked()){
    DOUT("in checked\n");
    static_eval = VALUE_NONE;
  }else{
    //mate three
    if(USE_MATE && abs(trans_value) < VALUE_EVAL_MAX){
      search_info.try_mate_++;
      DOUT("try mate three\n");
      //move = pos.mate_three(&ci);
      move = pos.mate_one(ci);
      if(move.value()){
        DOUT("mate three ok\n");
        search_info.ok_mate_++;
        return -value_mate(pos.ply());
      }
    }
    if(!pos.capture()
        && !pos.eval_is_empty()
        && !pos.eval_is_empty(pos.ply()-1)
        && pos.cur_move(pos.ply()-1) != move_pass()){
        //std::cout<<pos.eval()<<" "<<pos.eval(pos.ply()-1)<<std::endl;
        DOUT("update gain\n");
        DOUT("skip rep eval_org%d\n",pos.eval());
        const auto m = pos.cur_move(pos.ply()-1);
        const auto v = (pos.eval() - pos.eval(pos.ply()-1))/ FV_SCALE;
        gain.update(m.from(),m.to(),v);
    }
    //razoring
    if( USE_RAZORING
        && depth < 4 * DEPTH_ONE
        && nt == NOPV_NODE
        && beta > -VALUE_EVAL_MAX
        && static_eval + razor_margin(depth) <=alpha
        && !pos.trans_move().value()){
      search_info.try_razoring_++;
      DOUT("razoring\n");
      if(depth <= DEPTH_ONE
      && static_eval + razor_margin(3 * DEPTH_ONE) <= alpha){
        return full_quiescence<c>(pos,alpha,beta,0);
      }
      Value ralpha= alpha- razor_margin(depth);
      Value v = full_quiescence<c>(pos,ralpha,ralpha+1,0);
      if(v <= ralpha){
        DOUT("razoring ok\n");
        search_info.pruned_razoring_++;
        return v;
      }
    }
    //futility pruning in child node
    if( USE_FUTILITY
        && !pos.in_checked()
        && !pos.skip_null_
        && depth < 7 * DEPTH_ONE
        && nt == NOPV_NODE
        && abs(beta) < VALUE_EVAL_MAX
        && static_eval - get_futility_margin(depth) >= beta){
      search_info.pruned_futility_++;
      DOUT("futility in child\n");
      return static_eval - get_futility_margin(depth);
    }
    // null move pruning
    if( USE_NULL
        && !pos.skip_null_
        && depth >= 2 * DEPTH_ONE
        && nt == NOPV_NODE
        && abs(beta) < VALUE_EVAL_MAX
        && static_eval >= beta){

      search_info.try_null_++;
      ASSERT(pos.cur_move(pos.ply()-1) != move_pass());
      ASSERT(pos.eval() / FV_SCALE == static_eval);
      DOUT("null in\n");
      Depth null_depth = get_null_depth(depth)
                        - int(static_eval - beta)/piece_type_value_ex(PAWN)
                                                * DEPTH_ONE ;
      pos.skip_null_ = true;
      pos.do_null_move();
      value = -full_search<opp_color(c),NOPV_NODE>
        (pos, -beta, -(beta-1), null_depth, !cut_node);
      pos.undo_null_move();
      pos.skip_null_ = false;
      if(value >= beta){
        if(value >= VALUE_EVAL_MAX){
          value = beta;
        }
        if(depth < DEPTH_ONE * 12){
          search_info.pruned_null_++;
          DOUT("null cut\n");
          return value;
        }else{//TODO
          //verifiy_search need?
          DOUT("null verify\n");
          value = full_search<c,NOPV_NODE>(pos,alpha,beta,null_depth,false);
          if(value >= beta){
            search_info.pruned_null_++;
            DOUT("null verify ok\n");
            return value;
          }
        }
      }
    }
    //prob cut
    if( USE_PROB_CUT
        && nt == PV_NODE
        && depth >= 5 * DEPTH_ONE
        && !pos.skip_null_
        && abs(beta) < VALUE_EVAL_MAX){
      Value rbeta = std::min(beta + 200,VALUE_INF);
      Depth rdepth = depth - DEPTH_ONE - 3 * DEPTH_ONE;
      Sort sort(pos,pos.capture(),depth);
      Move move;
      DOUT("prob cut in\n");
      while((move = sort.next_move<c>()) != move_none()){
        if(pos.pseudo_is_legal(move,ci.pinned())){
          DOUT("prob cut loop\n");
          ASSERT(pos.move_is_tactical(move));
          pos.do_move(move,ci,pos.move_is_check(move,ci));
          value = -full_search<opp_color(c),NOPV_NODE>(pos,-rbeta,-rbeta+1,rdepth,!cut_node);
          pos.undo_move(move);
          if(value >= rbeta){
            DOUT("prob cut ok %d\n",value);
            return value;
          }
        }
      }
    }
    //iid
    if( USE_IID
        && ((nt == PV_NODE && depth >= 5 * DEPTH_ONE) || (nt == NOPV_NODE && depth >= 8 * DEPTH_ONE))
        && !pos.trans_move().value()
        && (nt == PV_NODE || static_eval + Value(256) >= beta)){
      Depth d = (depth - 2 * DEPTH_ONE - (nt == PV_NODE )) ? DEPTH_NONE : depth/4;
      DOUT("iid start\n");
      pos.skip_null_ = true;
      value = full_search<c,nt>(pos,alpha,beta,d,true);
      pos.skip_null_ = false;
      DOUT("iid end %d\n",value);
      if(abs(value) > VALUE_EVAL_MAX){
        return value;
      }
      const int iid_ply = pos.ply();
      const Move iid_move = pos.pv(iid_ply,iid_ply);
      if(iid_move.value() && pos.move_is_pseudo(iid_move)){
        pos.set_trans_move(iid_move);
      }
    }
  }
  DOUT("init sort\n");
  Sort sort(pos,depth);
  Move quies_searched[MAX_MOVE_NUM];
  int quies_sp= 0;
  int move_count = 0;
  const bool improving = pos.ply() <= 2 || pos.eval() >= pos.eval(pos.ply()-2);
  const bool singular_ok = USE_SINGULAR
                        && depth >= 8 * DEPTH_ONE
                        && pos.trans_move().value()
                        && !skip_move.value()
                        && trans_flag_is_exact_lower(trans_flag)
                        && trans_depth >= depth - 3 * DEPTH_ONE;

  Move best_move = move_none();
  Value best_value = VALUE_NONE;
  //singular extensionするとき消えてしまう
  const Move trans_move = pos.trans_move();

  DOUT("go loop\n");
 while((move = sort.next_move<c>()) != move_none()){
  //while(true){
    // move = sort.next_move<c>();
    // sort.lock_flag_ = false;
    // if(!move.value()){
    //   break;
    // }
    DOUT("loop in %s\n",move.str().c_str());
    ASSERT(move.is_ok(pos));

    ++move_count;

    if(move == skip_move){
      DOUT("skip move\n");
      continue;
    }

    Depth ext = DEPTH_NONE;
    const bool check = pos.move_is_check(move,ci);
    const bool pv_move = move_count == 1 && nt == PV_NODE;
    if(check){
      if(alpha <= -VALUE_EVAL_MAX){
        search_info.ext_++;
        ext = DEPTH_ONE;
      }else if(pos.see(move,Value(0),Value(1)) >= 0){
        DOUT("check ext\n");
        search_info.ext_++;
        ext = DEPTH_ONE;
      }
    }
    //singular extension
    if(singular_ok
    && move == trans_move
    && !ext
    && abs(trans_value) < VALUE_EVAL_MAX){
      Value rbeta = trans_value - int(depth);
      pos.set_skip_move(pos.ply(),move);
      pos.skip_null_ = true;
      DOUT("singular ext\n");
      value = full_search<c,NOPV_NODE>(pos,rbeta-1,rbeta,depth/2,cut_node);
      DOUT("singular end %d %d\n",value,rbeta);
      pos.skip_null_ = false;
      pos.set_skip_move(pos.ply(),move_none());
      if(value < rbeta){
        DOUT("singular ext %d < rbeta %d\n",value,rbeta);
        search_info.singuler_ext_++;
        ext = DEPTH_ONE;
      }
    }
    if(USE_HALF
        && pv_move
        && !pos.move_is_tactical(move)
        && !ext){
      search_info.half_ext_++;
      ext = DEPTH_ONE / 2;
    }
    ext = limit_extension(ext,pos.ply());
    Depth new_depth = depth - DEPTH_ONE + ext;
    if(nt == NOPV_NODE
    && !pos.move_is_tactical(move)
    && !check
    && !pos.in_checked()
    && best_value > -VALUE_EVAL_MAX){
      //move count based pruning
      if( USE_COUNT
      && depth < 16 * DEPTH_ONE
      && move_count >= get_move_count(improving,depth)){
        DOUT("move count\n");
        search_info.pruned_move_count_++;
        continue;
      }
      Depth predicted_depth = new_depth - get_reduction(nt == PV_NODE,improving,depth,move_count);
      //futility pruning
      if( USE_FUTILITY && predicted_depth < 7 * DEPTH_ONE){
        Value futility_value = static_eval + get_futility_margin(predicted_depth)
                             + Value(128) + gain.get(move.from(),move.to());
        if(futility_value <= alpha){
          search_info.pruned_futility_++;
          best_value = std::max(best_value, futility_value);
          DOUT("futility parent\n");
          continue;
        }
      }
      //negative see pruning
      if(USE_SEE && predicted_depth < 4 * DEPTH_ONE && pos.see(move,Value(-1),Value(0))<= -1){
        DOUT("negative see\n");
        search_info.pruned_see_++;
        continue;
      }
    }
    if(!pos.pseudo_is_legal(move,ci.pinned())){
      move_count--;
      continue;
    }
    if(!pos.move_is_tactical(move)){
      quies_searched[quies_sp++] = move;
    }
    pos.do_move(move,ci,check);

    bool do_full_depth = false;
    if(depth >= 3 * DEPTH_ONE
    && !pv_move
    && !pos.move_is_tactical(move)
    && move != trans_move
    && move != sort.killer_[0]
    && move != sort.killer_[1]){
      DOUT("LMR1\n");
      Depth reduce = get_reduction(nt == PV_NODE,improving,depth,move_count);
      if( nt == NOPV_NODE && cut_node){
        DOUT("reduce 1\n");
        reduce += DEPTH_ONE;
      }else if(history.get(move.from(),move.to()) < 0){
        DOUT("reduce 2\n");
        reduce += DEPTH_ONE / 2;
      }
      if(move == sort.killer_[2] || move == sort.killer_[3]){//counter moves
        DOUT("reduce 3\n");
        reduce = std::max(DEPTH_NONE,reduce - DEPTH_ONE);
      }
      pos.set_reduction(reduce);
      Depth d = std::max(new_depth - reduce,DEPTH_ONE);
      value = -full_search<opp_color(c),NOPV_NODE>(pos,-(alpha+1),-alpha,d,true);
      if(pos.tlp_stop_){
        goto undo_point;
      }
      DOUT("result1 %s = %d\n",move.str().c_str(),value);
      if(value > alpha && reduce >= 4 * DEPTH_ONE){
        const Depth d2 = std::max(new_depth - (2 * DEPTH_ONE), DEPTH_ONE);
        DOUT("LMR2\n");
        value = -full_search<opp_color(c),NOPV_NODE>(pos,-(alpha+1),-alpha,d2,true);
        DOUT("result2 %s = %d\n",move.str().c_str(),value);
        if(pos.tlp_stop_){
          goto undo_point;
        }
      }
      do_full_depth = (value > alpha && reduce != DEPTH_NONE);
      pos.set_reduction(DEPTH_NONE);
    }else{
      do_full_depth = !pv_move;
    }
    if(do_full_depth){
      DOUT("LMR3\n");
      value = -full_search<opp_color(c),NOPV_NODE>(pos,-(alpha+1),-alpha,new_depth,!cut_node);
      DOUT("result3 %s = %d\n",move.str().c_str(),value);
      if(pos.tlp_stop_){
        goto undo_point;
      }
    }
    if(nt == PV_NODE && (pv_move || (value > alpha && value < beta))){
      //ASSERT(alpha + 1 != beta);
      DOUT("LMR4\n");
      if(USE_HALF_RESEARCH
          && !ext
          && !pv_move
          && !pos.move_is_tactical(move)){
        ext = DEPTH_ONE / 2;
        ext = limit_extension(ext,pos.ply());
        if(ext){
          DOUT("re half\n");
        }
        new_depth += ext;
      }
      value = -full_search<opp_color(c),PV_NODE>(pos,-beta,-alpha,new_depth,false);
      DOUT("result4 %s = %d\n",move.str().c_str(),value);
      if(pos.tlp_stop_){
        goto undo_point;
      }
    }
    undo_point:
    pos.undo_move(move);
    if(root.stop() || pos.tlp_stop_){
      return VALUE_ZERO;
    }
    if(value > best_value){
      DOUT("best value update %d -> %d\n",best_value,value);
      best_value = value;
      best_move = move;
      pos.update_pv(move);
      if(value > alpha){
        DOUT("alpha value update %d -> %d\n",alpha,value);
        if(nt == PV_NODE && value < beta){
          alpha = value;
        }else{
          DOUT("beta cut %d -> %d\n",beta,value);
          goto beta_cut;
        }
      }
    }
#ifndef LEARN
#ifdef TLP
    if( g_thread.idle_
    && !pos.in_checked()
    && depth >= 6 * DEPTH_ONE){
      ASSERT(alpha < beta);
      ASSERT(move_count != 0);
      ASSERT(!sort.lock_flag_);
      pos.psort_[pos.ply()] = &sort;
      pos.pci_[pos.ply()] = &ci;
      pos.tlp_alpha_ = alpha;
      pos.tlp_beta_ = beta;
      pos.tlp_best_value_ = best_value;
      pos.tlp_depth_ = depth;
      pos.tlp_best_move_ = best_move;
      pos.tlp_cut_node_ = cut_node;
      pos.tlp_node_type_ = nt;
      DOUT("try tlp");
      //並列に探索
      if(tlp_split(&pos)){
        DOUT("tlp end");
        if(root.stop() || pos.tlp_stop_){
          return VALUE_ZERO;
        }
        value = pos.tlp_best_value_;
        if(value > best_value){
          best_value = value;
          best_move = pos.tlp_best_move_;
          if(value > alpha){
            if(value >= beta){
              goto beta_cut;
            }
            alpha = value;
          }
        }
        break;
      }
    }
#endif
#endif
  }
  DOUT("loop all end %d\n",move_count);
  if(!move_count){
    if(skip_move.value()){
      DOUT("skip\n");
      return alpha;
    }else{
      DOUT("mated\n");
      return value_mate(pos.ply());
    }
  }
  if(best_value == VALUE_NONE){
    DOUT("mated2\n");
    best_value = alpha;
  }
beta_cut:
  if(best_value >= beta){
    trans_flag = TRANS_LOWER;
  }else if(old_alpha != alpha){
    trans_flag = TRANS_EXACT;
  }else {
    trans_flag = TRANS_UPPER;
  }
  DOUT("store data v:%d m:%s flag:%d\n",best_value,best_move.str().c_str(),trans_flag);
  TT.store(pos,key,depth,best_move,best_value,trans_flag);
  if(best_value >= beta
  && !pos.move_is_tactical(best_move)
  && !pos.in_checked()){
    DOUT("update good move %s\n",best_move.str().c_str());
    pos.killer_[pos.ply()].update(best_move);
    const int v = int(depth * depth);
    for(int i = 0; i < quies_sp-1; ++i){
      history.update(quies_searched[i].from(),quies_searched[i].to(),-v);
    }
    //ASSERT(quies_searched[quies_sp-1] == best_move);
    history.update(best_move.from(),best_move.to(),v);
    const Move pre = pos.cur_move(pos.ply()-1);
    if(pre.value() && pre != move_pass()){
      counter.update(pre.from(),pre.to(),best_move);
    }
    if(pos.ply() >= 2){
      const Move pre2 = pos.cur_move(pos.ply()-2);
      if(pre2.value() && pre2 != move_pass() && pre == pos.trans_move(pos.ply()-1)){
        follow_up.update(pre2.from(),pre2.to(),best_move);
      }
    }
  }
  DOUT("search all end %d\n",best_value);
  return best_value;
}
template<Color c>Value full_quiescence(Position & pos, Value alpha, Value beta, int quies_ply){

  ASSERT(pos.is_ok());
  ASSERT(value_is_ok(alpha));
  ASSERT(value_is_ok(beta));
  ASSERT(alpha < beta);
  ASSERT(pos.turn() == c);

  DOUT("qsearch start\n");
  pos.init_pv();
  pos.inc_node();
  root.dec_node();
  pos.set_eval(STORED_EVAL_IS_EMPTY,pos.ply()+1);
  //root.searching_max_ply_ = std::max(root.searching_max_ply_,pos.ply());
  if((!pos.tlp_id_ && root.is_time_up()) || pos.tlp_stop_){
    return VALUE_ZERO;
  }
  Value old_alpha = alpha;
  Value best_value = VALUE_NONE;
  Value futility_base = VALUE_NONE;
  Move move;
  //mate distance pruning
  if(USE_DISTANCE){
    alpha = std::max( value_mate(pos.ply()  ), alpha);
    beta  = std::min(-value_mate(pos.ply()-1), beta );
    if(alpha >= beta){
      DOUT("qsearch mate_distance\n");
      return alpha;
    }
  }

  Depth trans_depth = DEPTH_NONE;
  Value trans_value = VALUE_ZERO;

  pos.set_trans_move(move_none());

  const auto key = pos.key();

  TransFlag trans_flag = TT.probe(pos,key,DEPTH_NONE,alpha,beta,trans_value,trans_depth);

  switch(trans_flag){
    case TRANS_UPPER:
      if(USE_TRANS && old_alpha + 1 == beta){
        DOUT("qsearch upper\n");
        return trans_value;
      }
      break;
    case TRANS_LOWER:
      if(USE_TRANS && old_alpha + 1 == beta){
        DOUT("qsearch lower\n");
        return trans_value;
      }
      break;
    case TRANS_EXACT:
      if(USE_TRANS){
        DOUT("qsearch exact\n");
        return trans_value;
      }
      break;
    default:
      break;
  }

  CheckInfo ci(pos);
  best_value = evaluate<c>(pos);
  if(pos.in_checked()){
    best_value = futility_base = VALUE_NONE;
  }else{
    futility_base = best_value + Value(128);
    if(best_value >= beta){
      move = move_none();
      TT.store(pos,key,DEPTH_QS,move,best_value,TRANS_LOWER);
      return best_value;
    }
    //mate three
    if(USE_MATE && abs(trans_value) < VALUE_EVAL_MAX){
      //move = pos.mate_three(&ci);
      move = pos.mate_one(ci);
      if(move.value()){
        DOUT("qsearch mate_three\n");
        return -value_mate(pos.ply());
      }
    }
  }
  if(pos.ply() >= MAX_PLY - 10){
    printf("quies near max ply\n");
    return evaluate<c>(pos);
  }
  Value value;
  Move best_move(0);
  Sort sort(pos,quies_ply);
  while((move = sort.next_move<c>())!=move_none()){
    bool check = pos.move_is_check(move,ci);
    //delta pruning
    if( USE_DELTA
    && old_alpha + 1 == beta
    && !pos.in_checked()
    && pos.trans_move() != move
    && !check
    && futility_base > -VALUE_EVAL_MAX){
      const PieceType pt = piece_to_piece_type(pos.square(move.to()));
      Value futility_value = futility_base + piece_type_value_ex(pt);
      if(move.is_prom()){
        const PieceType ppt = piece_to_piece_type(pos.square(move.from()));
        futility_value += piece_type_value_pm(ppt);
      }
      if(futility_value < beta){
        best_value = std::max(best_value,futility_value);
        continue;
      }
      if(futility_base < beta && pos.see(move,Value(0),Value(1)) <= Value(0)){
        best_value = std::max(best_value,futility_base);
        continue;
      }
    }
    bool evasion_ok = USE_EVASION
                      && pos.in_checked()
                      && best_value > -VALUE_EVAL_MAX
                      && !pos.move_is_tactical(move);
    if(old_alpha + 1 == beta
    && (!pos.in_checked() || evasion_ok)
    && pos.see(move,Value(-1),Value(0))<= -1){
      continue;
    }
    if(!pos.pseudo_is_legal(move,ci.pinned())){
      continue;
    }
    pos.do_move(move,ci,check);
    value = -full_quiescence<opp_color(c)>(pos,-beta,-alpha,quies_ply+1);
    pos.undo_move(move);
    if(root.stop() || pos.tlp_stop_){
      return alpha;
    }
    if(value > best_value){
      best_value = value;
      best_move = move;
      pos.update_pv(move);
      if(value > alpha){
        if(old_alpha + 1 != beta && value < beta){
          alpha = value;
        }else{
          TT.store(pos,key,DEPTH_QS,best_move,best_value,TRANS_LOWER);
          return value;
        }
      }
    }
  }
    DOUT("qsearch loop end\n");
  if(pos.in_checked() && best_value == VALUE_NONE){
    return value_mate(pos.ply());
  }
  if(old_alpha != alpha){
    trans_flag = TRANS_EXACT;
  }else{
    trans_flag = TRANS_UPPER;
  }
  TT.store(pos,key,DEPTH_QS,best_move,best_value,trans_flag);
  return best_value;
}
#ifdef TLP
template<Color c, NodeType nt>
Value tlp_full_search(Position & pos, Value alpha, Value beta, Depth depth, bool cut_node){

  ASSERT(pos.is_ok());
  ASSERT(pos.ply() > 0);
  ASSERT(value_is_ok(alpha));
  ASSERT(value_is_ok(beta));
  ASSERT(alpha < beta);
  ASSERT(c == pos.turn());
  ASSERT(nt == PV_NODE || ((nt == NOPV_NODE) && (alpha + 1 == beta)));

  if((!pos.tlp_id_ && root.is_time_up()) || pos.tlp_stop_){
    return VALUE_ZERO;
  }
  DOUT("tlp start \n");
  //OUT<<"start search\n";
  //printf("start split %d ",pos.tlp_id_);
  pos.init_pv();
  pos.inc_node();
  root.dec_node();
  Move quies_searched[MAX_MOVE_NUM];
  int quies_sp= 0;
  int move_count = 1;
  const bool improving = (pos.ply() <= 2) || pos.eval() >= pos.eval(pos.ply()-2);

  Move best_move = move_none();
  Value best_value = pos.tlp_parent_->tlp_best_value_;
  Value value = VALUE_NONE;
  Move skip_move = pos.skip_move(pos.ply());
  Move move(0);
  CheckInfo *ci = pos.tlp_parent_->pci_[pos.ply()];
  Sort *sort = pos.tlp_parent_->psort_[pos.ply()];
  ASSERT(sort != nullptr);
  ASSERT(ci != nullptr);

  //1手ずつ取ってくる
  for(;;){

    boost::mutex::scoped_lock lk(pos.tlp_parent_->tlp_lock);
    DOUT("tlp lock\n");
    if(pos.tlp_stop_ || root.stop()){
      lk.unlock();
      return VALUE_ZERO;
    }
    ASSERT(pos.tlp_parent_ != nullptr);
    ASSERT(!sort->lock_flag_);
    move = sort->next_move<c>();
    DOUT("tlp loop in %s\n",move.str().c_str());
    lk.unlock();
    if(move == move_none()){
      DOUT("tlp loop end \n");
      break;
    }
    ASSERT(move.is_ok(pos));

    ++move_count;

    if(move == skip_move){
      continue;
    }

    const bool pv_move = (nt == PV_NODE);
    Depth ext = DEPTH_NONE;
    bool check = pos.move_is_check(move,*ci);

    if(check && pos.see(move,Value(0),Value(1)) >= 0){
      ext = DEPTH_ONE;
    }
    //singular extension skiped in tlp_full_search
    if(USE_HALF
        && pv_move
        && !pos.move_is_tactical(move)
        && !ext){
      ext = DEPTH_ONE / 2;
    }
    ext = limit_extension(ext,pos.ply());

    Depth new_depth = depth - DEPTH_ONE + ext;
    if(nt == NOPV_NODE
        && !pos.move_is_tactical(move)
        && !check
        && !pos.in_checked()
        && best_value > -VALUE_EVAL_MAX){
      //move count based pruning
      if( USE_COUNT
          && depth < 16 * DEPTH_ONE
          && move_count >= get_move_count(improving,depth)){
        //OUT<<"move count\n";
        continue;
      }
      Depth predicted_depth = new_depth - get_reduction(nt == PV_NODE,improving,depth,move_count);
      //futility pruning
      if( USE_FUTILITY && predicted_depth < 7 * DEPTH_ONE){
        Value futility_value = (pos.eval()/FV_SCALE) + get_futility_margin(predicted_depth)
          + Value(128) + gain.get(move.from(),move.to());
        if(futility_value <= alpha){
          best_value = std::max(best_value, futility_value);
          continue;
        }
      }
      //negative see pruning
      if(USE_SEE && predicted_depth < 4 * DEPTH_ONE && pos.see(move,Value(-1),Value(0))<= -1){
        //OUT<<"negative see\n";
        continue;
      }
    }
    if(!pos.pseudo_is_legal(move,ci->pinned())){
      move_count--;
      continue;
    }
    if(!pos.move_is_tactical(move)){
      quies_searched[quies_sp++] = move;
    }
    //OUT<<"do move\n";
    pos.do_move(move,*ci,check);
    bool do_full_depth = false;
    if(depth >= 3 * DEPTH_ONE
        && !pv_move
        && !pos.move_is_tactical(move)
        && move != pos.trans_move()
        && move != sort->killer_[0]
        && move != sort->killer_[1]){
      //OUT<<"LMR\n";
      Depth reduce = get_reduction(nt == PV_NODE,improving,depth,move_count);
      if( nt == NOPV_NODE && cut_node){
        reduce += DEPTH_ONE;
      }else if(history.get(move.from(),move.to()) < 0){
        reduce += DEPTH_ONE / 2;
      }
      if(move == sort->killer_[2] || move == sort->killer_[3]){//counter moves
        reduce = std::max(DEPTH_NONE,reduce - DEPTH_ONE);
      }
      pos.set_reduction(reduce);
      Depth d = std::max(new_depth - reduce,DEPTH_ONE);

      alpha = pos.tlp_parent_->tlp_alpha_;

      value = -full_search<opp_color(c),NOPV_NODE>(pos,-(alpha+1),-alpha,d,true);
      if(value > alpha && reduce >= 4 * DEPTH_ONE){
        const Depth d2 = std::max(new_depth - (2 * DEPTH_ONE), DEPTH_ONE);
        value = -full_search<opp_color(c),NOPV_NODE>(pos,-(alpha+1),-alpha,d2,true);
      }
      do_full_depth = (value > alpha && reduce != DEPTH_NONE);
      pos.set_reduction(DEPTH_NONE);
    }else{
      do_full_depth = !pv_move;
    }
    if(do_full_depth){
      alpha = pos.tlp_parent_->tlp_alpha_;
      value = -full_search<opp_color(c),NOPV_NODE>(pos,-(alpha+1),-alpha,new_depth,!cut_node);
    }
    if(nt == PV_NODE && (pv_move || (value > alpha && value < beta))){
      //ASSERT(alpha + 1 != beta);
      value = -full_search<opp_color(c),PV_NODE>(pos,-beta,-alpha,new_depth,false);
    }
    pos.undo_move(move);

    if(pos.tlp_stop_ || root.stop()){
      return VALUE_ZERO;
    }
    if(value > best_value){
      pos.tlp_parent_->tlp_best_value_ = best_value = value;
      pos.tlp_parent_->tlp_best_move_ = best_move = move;
      pos.update_pv(move);
      if(value > alpha){
        pos.tlp_parent_->tlp_alpha_ = alpha =  value;
        if(value >= beta){
          boost::mutex::scoped_lock lk2(g_thread.lock_);
          if(pos.tlp_stop_ || root.stop()){
            lk2.unlock();
            return VALUE_ZERO;
          }
          g_thread.stop_num_++;
          //beta cutが起きたので、兄弟局面の探索を打ち切る
          DOUT("beta cut"); 
          for(int num = 0; num < g_thread.max_num_; ++num){
            if(pos.tlp_parent_->tlp_sibling_[num] && num != pos.tlp_id_){
              tlp_set_stop(pos.tlp_parent_->tlp_sibling_[num]);
            }
          }
          DOUT("beta cut end"); 
          lk2.unlock();
          return value;
        }
      }
    }
    //OUT<<"loop all end "<<move_count<<std::endl;
  }
  return best_value;
}
#endif
u64 perft(Position & pos, int ply, const int max_ply){

  //ASSERT(pos.is_ok());
  ASSERT(pos.is_ok());
  u64 ret = 0;
  MoveVal move[MAX_MOVE_NUM];
  MoveVal *m;
  if(ply >= max_ply){
    return 1;
  }
  m = move;
  CheckInfo ci(pos);
  if(pos.in_checked()){
    m = gen_move<EVASION_MOVE>(pos,m);
  }else{
    m = gen_move<CAPTURE_MOVE>(pos,m);
    m = gen_move<QUIET_MOVE>(pos,m);
    m = gen_move<DROP_MOVE>(pos,m);
    if(!pos.ply()){
      std::cout<<m-move<<std::endl;
    }
  }
  for(MoveVal * p = move; p < m; ++p){
    if(!pos.pseudo_is_legal(p->move,ci.pinned())){
      continue;
    }
    bool check = pos.move_is_check(p->move,ci);
    pos.do_move(p->move,ci,check);
    ret += perft(pos,ply+1,max_ply);
    pos.undo_move(p->move);
  }
  return ret;
}

template Value full_search<BLACK,PV_NODE>(Position & pos, Value alpha, Value beta, Depth depth, bool cut_node);
template Value full_search<WHITE,PV_NODE>(Position & pos, Value alpha, Value beta, Depth depth, bool cut_node);
template Value full_search<BLACK,NOPV_NODE>(Position & pos, Value alpha, Value beta, Depth depth, bool cut_node);
template Value full_search<WHITE,NOPV_NODE>(Position & pos, Value alpha, Value beta, Depth depth, bool cut_node);
template Value full_quiescence<BLACK>(Position & pos, Value alpha, Value beta, int quies_ply);
template Value full_quiescence<WHITE>(Position & pos, Value alpha, Value beta, int quies_ply);
#ifdef TLP
template Value tlp_full_search<BLACK,PV_NODE>(Position & pos, Value alpha, Value beta, Depth depth, bool cut_node);
template Value tlp_full_search<WHITE,PV_NODE>(Position & pos, Value alpha, Value beta, Depth depth, bool cut_node);
template Value tlp_full_search<BLACK,NOPV_NODE>(Position & pos, Value alpha, Value beta, Depth depth, bool cut_node);
template Value tlp_full_search<WHITE,NOPV_NODE>(Position & pos, Value alpha, Value beta, Depth depth, bool cut_node);
#endif

