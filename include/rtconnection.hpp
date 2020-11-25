
#pragma once



struct rtconnection
{
  loop_condition* loop;

  bool calibrating;
  bool calibrated;
  
  bool scenevid;
  bool eyevid;
  bool eyetracking;
  bool audio;

  bool fromfile;
  
  int64_t zeroed_pts;

  rtconnection();
  
  rtconnection(loop_condition* _loop);
  
  virtual ~rtconnection();
  
  virtual bool start( const size_t reqwid, const size_t reqhei, const size_t eye_reqwid, const size_t eye_reqhei )=0; 
  virtual bool start_fromvid( const std::string& s, const size_t reqwid, const size_t reqhei, const size_t eye_reqwid, const size_t eye_reqhei )=0;
  virtual void stop()=0;
  virtual int32_t copy_last_frame( timed_mat& retmat)=0;
  virtual int32_t copy_last_frame( const int64_t& pts, timed_mat& retmat, int64_t& allowed)=0;
  virtual void flush_frames( const long& pts )=0;
  virtual void flush_audio_frames( const long& pts )=0;
  virtual int copy_last_audio_frame( const long& pts, timed_audio_frame& taf )=0;
  virtual bool copy_last_audio_frame(  timed_audio_frame& taf )=0;
  virtual bool copy_last_eyepos( float& x, float& y, int64_t& newoffset, const int64_t& pts, int64_t& offset_us )=0;
  virtual bool calibrate()=0;
  virtual bool zero_timing()=0;
  virtual int32_t get_video_frame( timed_mat& retmat, const int64_t& zeroed_time_ms )=0;
  virtual int32_t get_audio_frame( timed_audio_frame& rettaf, const int64_t& zeroed_start_time_ms, const int64_t& len_ms )=0;
  virtual int32_t get_audio_frame( timed_audio_frame& rettaf, const int64_t& zeroed_start_time_ms )=0;
  virtual int32_t get_eyepos( float& x, float& y, const int64_t& zeroed_time_ms )=0;
  virtual int32_t get_calib_pos( float& x, float& y, const int64_t& zeroed_time_ms )=0;

  
  
};
