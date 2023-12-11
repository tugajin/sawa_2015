#include "myboost.h"
#include "init.h"
#include "usi.h"
#include "bonanza_method.h"
int main(){

  init_once();
#ifdef LEARN
  learn();
#else
  Usi usi;
  usi.loop();
#endif
  //make_book();
  //find_magic();
  return 0;
}
