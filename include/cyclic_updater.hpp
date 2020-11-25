



#pragma once


#define CU_THREAD_WAKEUP_MS 10

typedef double float64_t;

struct cyclic_updater
{
  uint64_t update_time_msec;
  int64_t currtime_msec;

  rtconnection* conn; 
  
  std::thread mythread;
  
  rteye2::Timer t;

  timed_mat mm;
  timed_audio_frame taf;

  std::vector<timed_audio_frame> tafs;
  
  float eyex, eyey;

  int32_t eyeres, vidres, audres;

  int64_t audio_buffering_lag_msec;
  
  int64_condvar cv;

  int64_t sample_offset_msec;

  bool looping;


  
  
  cyclic_updater( const int64_t& _update_time_msec, rtconnection*& _conn, const int64_t& _sample_offset_msec );
  
  
  void reset();

  int64_condvar* get_cv();

  int64_t wait_and_copy_newest_data( const int64_t& targ_msec, int32_t& _vidres, timed_mat& _mm, int32_t& _audres, timed_audio_frame& _taf, int32_t& _eyeres, float& _x, float& _y, const std::string& threadname="NONE" );
  
  
  void update();

  void loopfunct( );
  

  ~cyclic_updater();
  
  void start();
  void stop();
  
};

