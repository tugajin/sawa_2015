#include "type.h"
#include "futility.h"
#include <cmath>

Value futility_move_count[2][32];
Depth reduction[2][2][64][64];

void init_futility(){
  //move count table
  for(int d = 0; d < 32; d++){
    futility_move_count[0][d] = Value(2.4 + 0.222 * pow(d +  0.0,1.8));
    futility_move_count[1][d] = Value(3.0 +   0.3 * pow(d + 0.98,1.8));
  }
  //reduction table
  for(int hd = 1; hd < 64; hd++){
    for(int mc = 0; mc < 64; mc++){
      double   pv_red = log(double(hd)) * log(double(mc)) / 3.0;
      double nopv_red = 0.33 + log(double(hd)) * log(double(mc)) / 2.25;
      reduction[1][1][hd][mc] = (Depth) (  pv_red >= 1.0 ? floor(  pv_red * int(DEPTH_ONE)) : 0);
      reduction[0][1][hd][mc] = (Depth) (nopv_red >= 1.0 ? floor(nopv_red * int(DEPTH_ONE)) : 0);
      //improve does not use?
      reduction[1][0][hd][mc] = reduction[1][1][hd][mc];
      reduction[0][0][hd][mc] = reduction[0][1][hd][mc];
      if(reduction[0][0][hd][mc] > 2 * DEPTH_ONE){
        reduction[0][0][hd][mc] += DEPTH_ONE; 
      }else if(reduction[0][0][hd][mc] > 1 * DEPTH_ONE){
        reduction[0][0][hd][mc] += DEPTH_ONE / 2;
      }
    } 
  }
}
