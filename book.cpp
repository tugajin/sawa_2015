#include "myboost.h"
#include "book.h"
#include "position.h"
#include "move.h"
#include "random.h"
#include <fstream>
#include <vector>
#include <map>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>

Book book;

constexpr size_t book_entry_size = sizeof(Key)
                                 + sizeof(u32) * 16
                                 + sizeof(u32) * 16;

constexpr bool IGNORE_FURIBISHA = true;

struct BookEntry{
  Key key;
  u16 move[16];
  u16 count[16];
};

bool Book::open(){

  ifs_.open("book.bin");
  if(ifs_.fail()){
    return false;
  }
  ifs_.seekg(0,ifs_.end);
  size_ = ifs_.tellg() / book_entry_size;
  std::cout<<"book size "<<size_<<std::endl;
  return true;
}
bool Book::close(){

  ifs_.close();
  return true;
}
Move Book::move(Position & pos){

  BookEntry entry;
  const Key key = pos.key() ^ (Key(pos.hand_b().value()));
  ifs_.clear();
  if(get(key,entry)){
    std::cout<<"found in book\n";
    std::cout<<key<<std::endl;
    int best_score = 0;
    u32 best_move = 0;
    for(int i = 0; i < 16; i++){
      Move m;
      m.set(entry.move[i]);
      int score= entry.count[i];
      //振り飛車を選択しないようにする
      if(IGNORE_FURIBISHA){
        const auto col = pos.turn();
        const auto to = m.to();
        const auto file = square_to_file(to);
        const auto pt = piece_to_piece_type(pos.square(to));
        if( pos.rep_ply_ < 10
         && pt == ROOK
         && ((col == BLACK && file >= FILE_E)
          || (col == WHITE && file <= FILE_E))){
          score = 0;
        }
      }
      best_score += score;
      int myrand = random32(best_score);
      if(myrand < score){
        best_move = entry.move[i];
      }
    }
    Move m(best_move);
    return m;
  }
  return move_none();
}
bool Book::get(const Key key, BookEntry & e){

  //TODO
  //2分探索にする
  for(size_t i = 0; i != size_; ++i){
    if(ifs_.seekg(book_entry_size * i,ifs_.beg)){
      Key entry_key;
      ifs_.read((char *)&entry_key,sizeof(Key));
      if(key == entry_key){
        e.key = key;
        u32 move[16];
        u32 count[16];
        ifs_.read((char *)move,sizeof(u32) * 16);
        ifs_.read((char *)count,sizeof(u32) * 16);
        for(int j = 0; j < 16; j++){
          e.move[j] = move[j];
          e.count[j] = count[j];
        }
        return true;
      }
    }
  }
  return false;
}
void make_book(){

  std::ifstream ifs("book.sfen");
  std::ofstream ofs("book.bin",std::ios::binary);
  std::cout<<"\nstart make book\n";
  std::string buf;
  Position pos;
  u32 pos_num = 0;
  typedef std::map<Key, BookEntry> Table;
  Table tbl;
  int loop = 0;
  while(getline(ifs,buf)){
      std::cout<<"loop "<<loop++<<"\r";
      //std::cout<<buf<<std::endl;
      pos.init_hirate();
      const auto index2 = buf.find("moves ");
      if(index2 == std::string::npos){
      std::cout<<"error\n";
      exit(1);
      return;
      }
      //std::cout<<index2;
      const auto move_buf = buf.substr(index2 + 6);
      std::vector<std::string> split_buf;
      boost::algorithm::split(split_buf,move_buf,boost::algorithm::is_space());
      BOOST_FOREACH(const auto &m, split_buf){
        Move move;
        if(!move.set_sfen(m)){
          break;
        }
        if(pos.rep_ply_ > 50){
          break;
        }
        CheckInfo ci(pos);
        //std::cout<<m<<std::endl;
        //pos.print();
        //std::cout<<move.str(pos)<<std::endl;
        if(!move.is_ok(pos)){
          break;
        }
        const Key index = pos.key() ^ (Key(pos.hand_b().value()));
        Table::const_iterator it = tbl.find(index);
        if(it != tbl.end()){
          BookEntry tmp;
          tmp = it->second;
          int i;
          for(i = 0; i < 16; i++){
            if(tmp.move[i] == move.value()){
              //std::cout<<"exist\n";
              tmp.count[i]++;
              break;
            }
          }
          if(i == 16){
            //std::cout<<"not found\n";
            int j;
            for(j = 0; j < 16; j++){
              if(tmp.move[j] == 0){
                tmp.move[j] = move.value();
                tmp.count[j]++;
                break;
              }
            }
            if(j == 16){
              pos.print();
              for(int k = 0; k < 16; k++){
                Move m(tmp.move[k]);
                std::cout<<m.str()<<std::endl;
              }
            }
          }
          tbl[index] = tmp;
        }else{
          //std::cout<<"not found\n";
          BookEntry tmp;
          tmp.key = pos.key() ^ (Key(pos.hand_b().value()));
          for(int i = 0; i < 16; i++){
            tmp.move[i] = 0;
            tmp.count[i] = 0;
          }
          tmp.move[0] = move.value();
          tmp.count[0]++;
          tbl[tmp.key] = tmp;
        }

        pos.do_move(move,ci,pos.move_is_check(move,ci));
        const auto chk = pos.checker_bb();
        const auto mat = pos.material();
        pos.dec_ply();
        pos.set_checker_bb(chk,0);
        pos.set_material(mat);
        pos_num++;
      }
  }
  std::cout<<pos_num<<std::endl;
  //std::sort(tbl.begin(),tbl.end());
  std::cout<<"write\n";
  BOOST_FOREACH(auto & x,tbl){
    BookEntry tmp = x.second;
    Key key = tmp.key;
    u32 move[16],count[16];
    for(int i = 0; i < 16; i++){
      move[i]  = tmp.move[i];
      count[i] = tmp.count[i];
    }
    ofs.write((char*)&key,sizeof(Key));
    ofs.write((char*)move,sizeof(u32) * 16);
    ofs.write((char*)count,sizeof(u32) * 16);
  }
  std::cout<<"end\n";
  std::cout<<"book entry size is "<<tbl.size()<<std::endl;
}
