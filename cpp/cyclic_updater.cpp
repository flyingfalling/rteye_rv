

#include <cstdlib>
#include <cstdio>
#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>
#include <queue>
#include <deque>


#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>


#include <ctime>
#include <chrono> 
#include <unistd.h>


#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h> 

#include <opencv2/opencv.hpp>



#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <boost/fusion/container/vector/convert.hpp>
#include <boost/fusion/include/as_vector.hpp>



#include <boost/asio.hpp>


#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <Timer.hpp>

extern "C"
{
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
}



#include <utils.hpp>
#include <string_manip.hpp>

#include <timed_mat.hpp>
#include <loop_condition.hpp>
#include <rtconnection.hpp>

#include <librteye2.hpp>

#include <tobii_ffmpeg_connection.hpp>



#include <cyclic_updater.hpp>




cyclic_updater::cyclic_updater( const int64_t& _update_time_msec, rtconnection*& _conn, const int64_t& _sample_offset_msec )

  {
    sample_offset_msec= _sample_offset_msec;
    update_time_msec = _update_time_msec;
    conn=_conn;
    audio_buffering_lag_msec=50;
    eyeres=0, vidres=0, audres=0;
    looping=false;
  }





void cyclic_updater::reset()
  {
    
    bool result = conn->zero_timing();
    
    while( false == result )
      {
	std::this_thread::sleep_for( std::chrono::milliseconds( update_time_msec ) );

	result = conn->zero_timing();
      }
    
    currtime_msec=0;
    t.reset();
  }

  int64_condvar* cyclic_updater::get_cv()
  {
    return (&cv);
  }

  int64_t cyclic_updater::wait_and_copy_newest_data( const int64_t& targ_msec, int32_t& _vidres, timed_mat& _mm, int32_t& _audres, timed_audio_frame& _taf, int32_t& _eyeres, float& _x, float& _y, const std::string& threadname )
  {

    {
      std::unique_lock<std::mutex> lk( cv.mux );
      
      while( cv.val <= targ_msec && true == *(conn->loop) )
	{
	  cv.cv.wait_for( lk, std::chrono::milliseconds( CU_THREAD_WAKEUP_MS ) );
	}
      
      _vidres = vidres;
      _audres = audres;
      _eyeres = eyeres;
      _mm = mm;
      _taf = taf;
      _x = eyex;
      _y = eyey;
    }
    
    return (targ_msec + update_time_msec);
  }
  
  void cyclic_updater::update()
  {
    
    int64_t sample_time_msec = currtime_msec + sample_offset_msec;
    {
      std::unique_lock<std::mutex> lk( cv.mux );
      
      vidres = conn->get_video_frame( mm, sample_time_msec );
      
      audres = conn->get_audio_frame( taf, sample_time_msec );
      eyeres = conn->get_eyepos( eyex, eyey, sample_time_msec );

      
      cv.val = sample_time_msec;
    }
    
    
    cv.cv.notify_all();

  }

  void cyclic_updater::loopfunct( )
  {
    fprintf(stdout, "REV: entering cyclic updater's loopfunct, update regularity [%ld] msec\n", update_time_msec );
    reset();
    fprintf(stdout, "Finished CU reset (zeroing)\n");


    
    

    while( true == *(conn->loop) )
      {
	
	update();

	float64_t elapsed_usec = (t.elapsed() * 1e6);
	uint64_t currtime_usec = (currtime_msec * 1e3);
	int64_t currerr_usec = elapsed_usec - currtime_usec;
	int64_t updatetime_usec = (update_time_msec*1e3) - currerr_usec;
	
	currtime_msec += update_time_msec;
	if( updatetime_usec > 0 && true == *(conn->loop) )
	  {
	    std::this_thread::sleep_for( std::chrono::microseconds( updatetime_usec ) );
	  }
	
      }
    
    fprintf(stdout, "CU: Finished while loop...exiting?\n");
  }

  cyclic_updater::~cyclic_updater()
  {
    
  }
  
  void cyclic_updater::start()
  {
    mythread = std::thread( &cyclic_updater::loopfunct, this );
  }

  void cyclic_updater::stop()
  {
    fprintf(stdout, "REV: stopping the cyclic updater (REV setting LOOP to false, will stop all other RTC etc that use it)...\n");
    if( NULL == conn )
      {
	fprintf(stdout, "Won't try to set conn loop?\n");
      }
    else
      {
	
	fprintf(stdout, "Trying to set loop to false in CU\n");
	if( NULL == conn->loop )
	  {
	    fprintf(stdout, "Was null\n");
	  }
	else
	  {
	    *(conn->loop) = false;
	  }
	
      }
    fprintf(stdout, "Will join CU thread...\n");

    if( mythread.joinable() )
      {
	mythread.join();
      }
  }
