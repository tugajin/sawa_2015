#ifndef BOOK_H
#define BOOK_H

#include "type.h"
#include <iostream>
#include <fstream>

class Position;
class Move;
struct BookEntry;

class Book{
  public:
    Book(){
      size_ = 0;
    }
    ~Book(){
      close();
    }
    bool open();
    bool close();
    Move move(Position & pos);
  private:
    bool get(const Key key, BookEntry & e);
    std::ifstream ifs_;
    size_t size_;
};
extern void make_book();
extern Book book;

#endif
