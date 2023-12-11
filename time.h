#ifndef TIME_H
#define TIME_H

#include <chrono>

class Timer{
  public:
    Timer(){
      start();
      result_ = 0;
    }
    void start(){
      timer_ = std::chrono::system_clock::now();
      result_ = 0;
    }
    int elapsed(){
      //ミリ秒を返す
      return std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::system_clock::now() - timer_).count();
    }
    int result_;
  private:
    decltype(std::chrono::system_clock::now()) timer_;
};

#endif
