#pragma once




struct tobii_ffmpeg_connection
  : public rtconnection
{

   

  
  std::thread keepalive_data_thread;
  std::thread keepalive_scene_thread;
  std::thread keepalive_eye_thread;
  
  std::thread scene_thread1_receive_socket;
  std::thread scene_thread2_read_send_receive_mpegts;
  std::thread vid_thread1_BGR_conversion;
  
  std::thread aud_thread1_recv_decode_mpegts;
  
  std::thread eyepos_thread1_receive_socket;
  std::thread eyepos_thread2_parse_update_eyestate;

  std::thread eyevid_thread1_receive_socket;
  std::thread eyevid_thread2_read_send_receive_mpegts;
  

  std::thread vidfile_thread1_read;

  int datsocket;
  int vidsocket;
  int eyesocket;
  buffered_socket buf_vid_socket;
  buffered_socket buf_dat_socket;
  buffered_socket buf_eye_socket;
  
  std::string KA_DATA_MSG; 
  std::string KA_VIDEO_MSG; 
  std::string KA_EYE_MSG; 
  std::string STOP_DATA_MSG; 
  std::string STOP_VIDEO_MSG; 
  std::string STOP_EYE_MSG; 
  
  bool_condvar scene2_cv;
  bool_condvar vid1_cv;
  bool_condvar aud1_cv;
  bool_condvar eye2_cv;
  
  bool_condvar eyevid_cv;
  bool_condvar eyevid_decode_cv;
  
  data_pkg_buffer mydpb;
  
  mpegts_parser tsparser;
  mpegts_parser eyevid_tsparser;
  

  tobii_data_dumper dumper;

  
  

  
  int port; 
  std::string ip4addr; 

  int64_t pts_to_msec( const int64_t& pts )
  {
    int64_t val = (pts * 1e3) / tsparser.pts_timebase_hz_sec;
    return val;
  }
  
  int64_t msec_to_pts( const int64_t& msec )
  {
    
    int64_t val = (msec * tsparser.pts_timebase_hz_sec); 
    
    val /= 1e3;
    
    return val;
  }

  int64_t msec_to_pts_zeroed( const int64_t& msec )
  {
    int64_t val = msec_to_pts( msec );
    
    val += zeroed_pts;
    return val;
  }

  int64_t msec_to_audio_samples( const int64_t& msec )
  {
    int64_t val = (msec * tsparser.audio_rate_hz_sec) / 1e3;
    return val;
  }

  
  void simulate_scenecamera( const size_t fps, const std::string& vidfname, loop_condition& loop );
  
  tobii_ffmpeg_connection( loop_condition* _loop, const bool _scenevid=true, const bool _eyetracking=true, const bool _audio=true, const bool _eyevid=false, const std::string& ip="192.168.71.50", const int _port=49152, const std::string& _dumpfile="" )
    : rtconnection( _loop )
  {
    
     
    
    KA_DATA_MSG = "{\"type\": \"live.data.unicast\", \"key\": \"some_GUID\", \"op\": \"start\"}";
    KA_VIDEO_MSG = "{\"type\": \"live.video.unicast\", \"key\": \"some_other_GUID\", \"op\": \"start\"}";
    KA_EYE_MSG = "{\"type\": \"live.eyes.unicast\", \"key\": \"some_third_GUID\", \"op\": \"start\"}";

    STOP_DATA_MSG = "{\"type\": \"live.data.unicast\", \"key\": \"some_GUID\", \"op\": \"stop\"}";
    STOP_VIDEO_MSG = "{\"type\": \"live.video.unicast\", \"key\": \"some_other_GUID\", \"op\": \"stop\"}";
    STOP_EYE_MSG = "{\"type\": \"live.eyes.unicast\", \"key\": \"some_third_GUID\", \"op\": \"stop\"}";
    
    scenevid = _scenevid;
    eyetracking = _eyetracking;
    audio = _audio;
    eyevid = _eyevid;
    
    vidsocket=-1;
    datsocket=-1;
    eyesocket=-1;
    
    ip4addr = ip;
    port=_port;
    fromfile=false;
    calibrating=false;
    zeroed_pts=0;
    
    dumper.init( _dumpfile );
        
    fprintf(stdout, "TOBIIFFMPEG CONTOR: SCENEVID [%s]    AUDIO [%s]   EYETRACKING [%s]   EYE VIDEO [%s]   (dumping raw to files? [%s])\n", scenevid?"YES":"NO", audio?"YES":"NO", eyetracking?"YES":"NO", eyevid?"YES":"NO", dumper.empty()?"NO":"YES" );
  }

  ~tobii_ffmpeg_connection()
  {
    
  }

  bool copy_last_eyepos( float& x, float& y, int64_t& newoffset, const int64_t& pts, int64_t& offset )
  {
    if( eyetracking)
      {
	bool flushupto=true;
	bool got = mydpb.get_gp_of_pts( pts, offset, x, y, flushupto );
	return got;
      }
    else
      {
	
	return false;
      }
  }
  
  void conn_socket_ip()
  {
    
    fprintf(stdout, "TOBII Will connect to ip [%s] and port [%d]\n", ip4addr.c_str(), port );
    
    vidsocket = client_connect_udp_socket( ip4addr, port );
    datsocket = client_connect_udp_socket( ip4addr, port );
    eyesocket = client_connect_udp_socket( ip4addr, port );

    
    
    size_t vidmesgsize=4*1024; 
    size_t vidtrigger = 10*vidmesgsize;

    size_t eyemesgsize=4*1024; 
    size_t eyetrigger = 10*vidmesgsize;
    
    size_t datmesgsize=1024; 
    size_t dattrigger = datmesgsize;
    
    
    buf_vid_socket.init( vidsocket, vidtrigger, vidmesgsize );
    buf_dat_socket.init( datsocket, dattrigger, datmesgsize );
    buf_eye_socket.init( eyesocket, eyetrigger, eyemesgsize );
    
    fprintf(stdout, "TOBII Connected to ip [%s] and port [%d] (vid socket is [%d], dat socket is [%d], eye vid socket [%d]\n", ip4addr.c_str(), port, vidsocket, datsocket, eyesocket );
  }

  void close_socket_ip()
  {
    
    fprintf(stdout, "TOBII Will close sockets to ip [%s] and port [%d]\n", ip4addr.c_str(), port );
    
    close( vidsocket );
    close( datsocket );
    close( eyesocket );

    
        
    fprintf(stdout, "TOBII closed connection to ip [%s] and port [%d] (vid socket is [%d], dat socket is [%d], eyesocket is [%d]\n", ip4addr.c_str(), port, vidsocket, datsocket, eyesocket );
  }
  
  bool calibrate()
  {
    calibrating = true;
    
    std::string base_url = ip4addr;
    tobii_REST tr( base_url );
    
    fprintf(stdout, "Starting calibration of TOBII eyetracker to [%s]\n", base_url.c_str() );
    
    std::string projid = tr.create_tobii_project();
    fprintf(stdout, "REV: created tobii project ID [%s]\n", projid.c_str());
    std::string partid = tr.create_tobii_participant( projid );
    fprintf(stdout, "REV: created tobii participant ID [%s]\n", partid.c_str());
    std::string caliid = tr.create_tobii_calibration( projid, partid );
    fprintf(stdout, "REV: created and starting calibration for calibration id [%s]\n", caliid.c_str() );
    
    tr.start_tobii_calibration( caliid );

        
    fprintf(stdout, "Will now wait for status\n");
    
    size_t maxtimeouts=30;
    std::string finalstatus = wait_for_status( maxtimeouts,
					       base_url,
					       "/api/calibrations/" + caliid + "/status",
					       "ca_state",
					       {"failed", "calibrated"} );

    calibrating = false;
    
    if( is_same_string( finalstatus, "failed" ) ) 
      {
	fprintf(stderr, "REV: Tobii calibration FAILED...\n");
	calibrated=false;
	return false;
      }
    else if( is_same_string( finalstatus, "calibrated" ) )
      {
	fprintf(stderr, "REV: Tobii calibrated SUCCEEDED\n");
	calibrated=true;
	return true;
      }
    else
      {
	fprintf(stderr, "REV: Tobii calibration UNKNOWN (returning true anyways)\n");
	calibrated=false;
	return false;
      }

    
  }
  
  
  void start_eyetracking();
  void start_scenevid();
  void start_audio();

  bool start( const size_t reqwid, const size_t reqhei, const size_t eye_reqwid=0, const size_t eye_reqhei=0 );

  bool start_fromvid( const std::string& vidfname, const size_t reqwid, const size_t reqhei , const size_t eye_reqwid=0, const size_t eye_reqhei=0 ); 
  
  
 
  void stop()
  {
    fprintf(stdout, "In tobii stop(), will join all threads, and exit\n");
    *loop = false;
    if( true == fromfile )
      {
	fprintf(stdout, "Joining vidfile thread1 read\n");
	if( vidfile_thread1_read.joinable() )
	  {
	    vidfile_thread1_read.join();
	  }
	return;
      }
    
    fprintf(stdout, "In tobii stop(), will join keepalive thread, recv thread, uninit scene loop, close socket.\n");
    
    
    
    std::this_thread::sleep_for( std::chrono::milliseconds(500) );
    

    if( scenevid )
      {
	if( vid_thread1_BGR_conversion.joinable() )
	  {
	    vid_thread1_BGR_conversion.join();
	  }
      }

    fprintf(stdout, "Finished closing bgr loop\n");
    
    fprintf(stdout, "Closed eyevid loop\n");
    
    if(audio)
      {
	if (aud_thread1_recv_decode_mpegts.joinable())
	  {
	    aud_thread1_recv_decode_mpegts.join();
	  }
      }

    fprintf(stdout, "Finished closing audio...\n");

    if( scene_thread2_read_send_receive_mpegts.joinable() )
      {
	scene_thread2_read_send_receive_mpegts.join();
      }
    fprintf(stdout, "Joined scene thread2\n");
    
    if( scene_thread1_receive_socket.joinable())
      {
	scene_thread1_receive_socket.join();
      }
    fprintf(stdout, "Joined scene thread1\n");
    
    if( keepalive_scene_thread.joinable())
      {
	keepalive_scene_thread.join();
	kill_stream_loop( vidsocket, STOP_VIDEO_MSG, *loop );
      }
    fprintf(stdout, "Joined keepalive scene thread1\n");

    if(eyevid)
      {
	
	if( eyevid_thread2_read_send_receive_mpegts.joinable() )
	  {
	    eyevid_thread2_read_send_receive_mpegts.join();
	  }
	fprintf(stdout, "Joined eyevid thread2\n");

	if( eyevid_thread1_receive_socket.joinable())
	  {
	    eyevid_thread1_receive_socket.join();
	  }
	fprintf(stdout, "Joined eyevid thread1\n");
	if( keepalive_eye_thread.joinable())
	  {
	    keepalive_eye_thread.join();
	    kill_stream_loop( eyesocket, STOP_EYE_MSG, *loop );
	  }
	fprintf(stdout, "Joined keepalive eyevid thread1\n");
      }
    
    
    
    fprintf(stdout, "Finished closing scene stuff...\n");
    
    if( eyetracking )
      {
	if( eyepos_thread2_parse_update_eyestate.joinable() )
	  {
	    eyepos_thread2_parse_update_eyestate.join();
	  }
	if( eyepos_thread1_receive_socket.joinable() )
	  {
	    eyepos_thread1_receive_socket.join();
	  }
	if( keepalive_data_thread.joinable() )
	  {
	    keepalive_data_thread.join();
	    kill_stream_loop( datsocket, STOP_DATA_MSG, *loop );
	  }
      }

    fprintf(stdout, "Finished closing eyetracking...\n");
    
    close_socket_ip();
    
    fprintf(stdout, "Finished closing sockets...\n");
  }
  
  bool zero_timing();
  
  int32_t get_video_frame( timed_mat& retmat, const int64_t& zeroed_time_ms );
  int32_t get_audio_frame( timed_audio_frame& rettaf, const int64_t& zeroed_start_time_ms, const int64_t& len_ms );
  int32_t get_audio_frame( timed_audio_frame& rettaf, const int64_t& zeroed_start_time_ms );
  int32_t get_eyepos( float& x, float& y, const int64_t& zeroed_time_ms );
  int32_t get_calib_pos( float& x, float& y, const int64_t& zeroed_time_ms );
  
  int32_t copy_last_frame( timed_mat& retmat );
  int32_t copy_last_frame( const int64_t& pts, timed_mat& retmat, int64_t& allowed);
  int copy_last_audio_frame( const long& pts, timed_audio_frame& taf);
  bool copy_last_audio_frame(  timed_audio_frame& taf );
  void flush_frames( const long& pts );
  void flush_audio_frames( const long& pts );
  
  
  void scene_threadfunct1_receive_socket_packets( loop_condition& loop, bool_condvar& scene2_condvar, buffered_socket& mybufsock, mpegts_parser& mytsparser );
  void scene_threadfunct2_read_send_receive_mpegts_packets( loop_condition& loop, bool_condvar& scene2_condvar, bool_condvar& vid1_condvar, bool_condvar& aud1_condvar, mpegts_parser& mytsparser );

  
  void eyevid_threadfunct1_receive_socket_packets( loop_condition& loop, bool_condvar& eyevid_condvar, buffered_socket& mybufsock, mpegts_parser& mytsparser  );
  void eyevid_threadfunct2_read_send_receive_mpegts_packets( loop_condition& loop, bool_condvar& eyevid_condvar, bool_condvar& eyevid_decode_condvar, mpegts_parser& mytsparser  );
  
  void vid_threadfunct1_BGR_conversion( loop_condition& loop, mpegts_parser& mytsparser, bool_condvar& vid1_condvar );
  
  
  void aud_threadfunct1_receive_decode_mpegts_packets( loop_condition& loop, mpegts_parser& mytsparser, bool_condvar& aud1_condvar );
  
  
  void eyepos_threadfunct1_receive_socket_packets( loop_condition& loop, bool_condvar& eyepos2_condvar, buffered_socket& mybufsock );
  void eyepos_threadfunct2_parse_update_eyestate( loop_condition& loop, bool_condvar& eyepos2_condvar, buffered_socket& mybufsock, data_pkg_buffer& dpb );
  
    
  
}; 
