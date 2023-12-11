#include <stdlib.h>
#include <cmath>
#include <limits.h>
#include"random.h"

u64 random_for_magic(){

  u64 v = 0, n = 1;
  for(int i = 0; i < 64; i++){
    if(random64()%1000<150)
      v += n;
    n = n << 1;
  }
  return v;
}
u64 random64(){

  static u64 x=123456789,y=362436069,z=521288629,w=88675123;
  u64 t;
  t=(x^(x<<11));x=y;y=z;z=w;
  return( w=(w^(w>>19))^(t^(t>>8)) );
}
u32 random32(){

  static u64 x=123456789,y=362436069,z=521288629,w=88675123;
  u64 t;
  t=(x^(x<<11));x=y;y=z;z=w;
  return u32( w=(w^(w>>19))^(t^(t>>8)) );
}
int random32(const int n){
  ASSERT(n > 0);
  double r;
  ASSERT(n > 0);
  r = double(random32()) / (double(UINT_MAX) + 1.0);
  r = floor(r*double(n));
  return int(r);
}
void init_rand(){

  srand(u32(time(NULL)));
  int lim = rand();
  std::cout<<"init rand...("<<lim<<")";
  for(int i =0; i < lim; i++){
    random32();
  }
  std::cout<<"end\n";
}

