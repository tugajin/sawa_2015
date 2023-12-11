#ifndef INIT_H
#define INIT_H

class Position;

extern void init_once();//一回だけ呼ぶ
extern void init_game();//対局が始まるたびに呼ぶ
extern void init_turn(Position & pos);//各手番ごとに呼ぶ
extern void exit_game();//対局が終わったら呼ぶ

#endif
