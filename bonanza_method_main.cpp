#include "myboost.h"
#include "bonanza_method.h"
#include "futility.h"
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/thread.hpp>
#include <boost/ref.hpp>

std::ifstream g_read_record_stream;//read record.sfen
std::ofstream g_write_of_stream;//output of

boost::mutex g_read_lock;

int g_record_num = 0;

const u64 MAX_ITERATE = -1;
const u64 MAX_STEP = 32;
const bool IS_INIT = false;

void learn(){

  fflush(stdout);
  setbuf(stdout,NULL);
  BonanzaMethod worker;
  std::cout<<"Start Bonanza Learning\n";
  std::cout<<boost::format("iterate %llu step %llu penalty %lf depth %d init %d TLP %d\n")
  % MAX_ITERATE % MAX_STEP % FV_PENALTY % (LEARN_DEPTH / DEPTH_ONE) % IS_INIT % BonanzaMethod::TLP_NUM;
  if(IS_INIT){
    clear_weight();
  }
  print_material();
  worker.init();
  for(u64 iterate = 0; iterate < MAX_ITERATE; iterate++){
    worker.phase_one(iterate);
    worker.phase_two(iterate);
  }
  std::cout<<"end\n";
}
void BonanzaMethod::init(){

  sum_material_ = VALUE_ZERO;
  PIECE_TYPE_FOREACH(p){
    if(p != KING){
      sum_material_ += p_value[p];
    }
  }
  //g_read_record_stream.open("record.sfen");
  g_read_record_stream.open("学習用棋譜.sfen");
  if(g_read_record_stream.fail()){
    std::cout<<"not found record.sfen\n";
    exit(1);
  }
  g_record_num = 0;
  std::string buf;
  while(getline(g_read_record_stream,buf)){
    g_record_num++;
  }
  std::cout<<"record num "<<g_record_num<<"\n";
}
u32 BonanzaMethod::phase_one(const u64 iterate){

  g_read_record_stream.clear();
  g_read_record_stream.seekg(0,std::ios_base::beg);

  phase_one_[0].init_info(0);
#ifdef TLP
  for(int i = 0; i < BonanzaMethod::TLP_NUM; i++){
    phase_one_[i].init_info(i);
  }
#endif
  std::cout<<"Phase One "<<iterate<<std::endl;

#ifdef PAIR
  std::cout<<"PAIR "<<iterate<<std::endl;
#endif

#ifdef TLP
  boost::thread *th[BonanzaMethod::TLP_NUM];
  for(int i = 1; i < BonanzaMethod::TLP_NUM; i++){
    th[i] = new boost::thread(&PhaseOne::worker,&phase_one_[i]);
  }
#endif
  phase_one_[0].worker();
#ifdef TLP
  //wait finish th
  for(int i = 1; i < BonanzaMethod::TLP_NUM; i++){
    th[i]->join();
    phase_one_[0].nodes_ += phase_one_[i].nodes_;
    delete th[i];
  }
#endif
  phase_one_[0].print();
  return 1;
}
u32 BonanzaMethod::phase_two(const u64 iterate){

  std::cout<<"Phase Two "<<iterate<<std::endl;
#ifdef PAIR
  std::cout<<"PAIR "<<iterate<<std::endl;
#endif
  for(u64 i = 0; i < MAX_STEP; i++){
#ifdef TLP
    boost::thread *th[BonanzaMethod::TLP_NUM];
    for(int i = 1; i < BonanzaMethod::TLP_NUM; i++){
      phase_two_[i].init_info(i);
      th[i] = new boost::thread(&PhaseTwo::worker,&phase_two_[i]);
    }
#endif
    phase_two_[0].init_info(0);
    phase_two_[0].worker();
#ifdef TLP
  //wait finish th
    for(int i = 1; i < BonanzaMethod::TLP_NUM; i++){
      //std::cout<<"fin"<<i<<std::endl;
      th[i]->join();
      delete th[i];
    }
    merge_phase_two();
#endif
    phase_two_[0].update_weight();
    std::cout<<".";
    if(i&&i % 10 == 0){
      std::cout<<i<<"\n";
    }
  }
  phase_two_[0].print();
  write_weight();
  return 1;
}
u32 BonanzaMethod::write_weight(){

  std::ofstream mat_ofs("material.txt2");
  PIECE_TYPE_FOREACH(p){
    mat_ofs<<piece_type_value(p)<<std::endl;
  }
  FILE *fp;
  if((fp = fopen("fv.bin2","wb")) == NULL){
    std::cout<<"can't open fv.bin2\n";
    exit(1);
  }
  size_t size = int(SQUARE_SIZE) * int(fe_end) * int(fe_end);
  if(fwrite(pc_on_sq,sizeof(Kvalue),size,fp) != size){
    std::cout<<"fail write kkp\n";
    exit(1);
  }
  size = int(SQUARE_SIZE) * int(SQUARE_SIZE) * int(kkp_end);
  if(fwrite(akkp,sizeof(Kvalue),size,fp) != size){
    std::cout<<"fail write kpp\n";
    exit(1);
  }
  fclose(fp);
  std::cout<<"write fv.bin2\n";
  return 1;
}
void BonanzaMethod::merge_phase_two(){

  for(int i = 1; i < BonanzaMethod::TLP_NUM; i++){
    phase_two_[0].target_ += phase_two_[i].target_;
    phase_two_[0].position_num_ += phase_two_[i].position_num_;
  }
  PIECE_TYPE_FOREACH(pt){
    for(int i = 1; i < BonanzaMethod::TLP_NUM; i++){
      phase_two_[0].grad_.piece_[pt] += phase_two_[i].grad_.piece_[pt];
    }
  }
  for(int i = 0; i < int(SQUARE_SIZE); i++){
    for(int j = 0; j < int(SQUARE_SIZE); j++){
      for(int k = 0; k <int(kkp_end); k++){
        for(int l = 1; l < BonanzaMethod::TLP_NUM; l++){
          phase_two_[0].grad_.kkp_[i][j][k] += phase_two_[l].grad_.kkp_[i][j][k];
        }
      }
    }
  }
  for(int i = 0; i < int(SQUARE_SIZE); i++){
    for(int j = 0; j < int(fe_end); j++){
      for(int k = 0; k <int(fe_end); k++){
        for(int l = 1; l < BonanzaMethod::TLP_NUM; l++){
          phase_two_[0].grad_.pc_on_sq_[i][j][k] += phase_two_[l].grad_.pc_on_sq_[i][j][k];
        }
      }
    }
  }
}

