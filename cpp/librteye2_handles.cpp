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

#include <loop_condition.hpp>
#include <timed_mat.hpp>

#include <rtconnection.hpp>

#include <librteye2.hpp>

#include <tobii_ffmpeg_connection.hpp>



#include <cyclic_updater.hpp>

#include <librteye2_handles.hpp>



struct rteye_wrapper
{
  cyclic_updater* cu;
  rtconnection* rtc;
  
  loop_condition loop;


  int32_t vidres;
  int32_t audres;
  int32_t eyeres;

  timed_mat tvf;
  timed_audio_frame taf;
  float eyex;
  float eyey;

  float calibx, caliby;

  std::thread calibthread;
  
  
  rteye_wrapper()
    : cu(NULL), rtc(NULL)
  {
    
  }

  int64_t update( const int64_t targ_msec )
  {
    if( cu && rtc )
      {
	rteye2::Timer tt;
	
	int64_t nextupdate = cu->wait_and_copy_newest_data( targ_msec, vidres, tvf, audres, taf, eyeres, eyex, eyey, "get_and_display" );
	fprintf(stdout, "RTEYE C++ update took [%lf] msec\n", tt.elapsed()*1000);
	
	
	return nextupdate;
      }
    else
      {
	fprintf(stderr, "REV: cu or rtc is null...exiting\n");
	exit(1);
      }
  }


  cv::Mat get_scene_frame()
  {
    
    if( vidres >= 0 )
      {
	return tvf.mat;
      }
    else
      {
	
	fprintf(stderr, "xxxx   BIG HUGE WARNING    ----- REV: couldn't get mat...returning empty matrix...?\n");
	return cv::Mat();
      }
  }

  int64_t get_scene_frame_idx()
  {
    if( vidres >= 0 )
      {
	return tvf.mt.idx;
      }
    else
      {
	fprintf(stderr, "REV: couldn't get mat...returning fake [[IDX]] -1000000000\n");
	return -100000000;
      }
  }

  int64_t get_scene_frame_pts()
  {
    if( vidres >= 0 )
      {
	return tvf.mt.pts;
      }
    else
      {
	fprintf(stderr, "REV: couldn't get mat...returning fake [[PTS]] -1000000000\n");
	return -1000000000;
      }
  }
  
  float get_eyex()
  {
    if( eyeres > 0 ) 
      {
	return eyex;
      }
    else
      {
	return -10000000;
      }
  }

  float get_eyey()  
  {
    if( eyeres > 0 )
      {
	return eyey;
      }
    else
      {
	return -10000000;
      }
  }

  void update_calib_pos(  const int64_t targ_msec )
  {
    if( !rtc )
      {
	return;
      }
    
    if( true == rtc->calibrating )
      {
	float x,y;
	bool m2d = rtc->get_calib_pos( x, y, targ_msec-10 );
	if(m2d)
	  {
	    calibx = x;
	    caliby = y;
	  }
	else
	  {
	    calibx = -1000000;
	    caliby = -1000000;
	  }
      }
  }
  
  float get_calib_x()  
  {
    if( rtc->calibrating )
      {
	return calibx;
      }
    else
      {
	return -10000000;
      }
  }

  float get_calib_y()  
  {
     if( rtc->calibrating )
      {
	return caliby;
      }
    else
      {
	return -10000000;
      }
  }


  void calibrate()
  {
    
    rtc->calibrating = true;

    calibthread = std::thread( &rtconnection::calibrate, rtc );
    
  };

  void endcalib()
  {
    calibthread.join();
  }
  
  
  
  bool connect( const std::string& ipaddr, const int port, const bool doscene, const bool doeye, const bool dosound, const bool doeyevid, const std::string& dumpfile, const int reqwid, const int reqhei, const int eyewid, const int eyehei )
  {
    loop.set_true();
    
    rtc = new tobii_ffmpeg_connection( &loop, doscene, doeye, dosound, doeyevid, ipaddr, port, dumpfile );

    fprintf(stdout, "Succeeded in allocating tobii...will now start it using reqwid/reqhei\n");
    
    
    bool started = rtc->start( reqwid, reqhei, eyewid, eyehei );

    fprintf(stdout, "Got past rtconn START\n");
    
    if( false == started )
      {
	fprintf(stderr, "REV: Failure starting rt_connection start(). Deleting RTC and CU to retry...\n");
	
	loop.set_false();

	if( rtc )
	  {
	    delete rtc;
	    rtc = NULL;
	  }
	
	if( cu )
	  {
	    delete cu;
	    cu = NULL;
	  }

	
	

	return false;
      }
    else
      {
	uint64_t update_time_msec = 10;
	int64_t sample_offset_msec = -200;
	
	cu = new cyclic_updater( update_time_msec, rtc, sample_offset_msec );
	cu->start();
	
	return true;
      }
  }
  
  
  ~rteye_wrapper()
  {
    loop.set_false();
    
    if( cu )
      {
	cu->stop();
      }
    if( rtc )
      {
	rtc->stop();
      }

    if( cu )
      {
	delete cu;
	cu = NULL;
      }
    
    if( rtc )
      {
	delete rtc;
	rtc = NULL;
      }
  }

  
};


void* rteye_wrapper_init( const std::string& ipaddr, const int port, const bool doscene, const bool doeye, const bool dosound, const bool doeyevid, const int reqwid, const int reqhei, const int eyewid, const int eyehei )
{

  fprintf(stdout, "Attempting WRAPPER_INIT\n");
  
  rteye_wrapper* rtw = new rteye_wrapper;
  std::string dumpfile="";

  fprintf(stdout, "Attempting RTW->CONNECT\n");

  bool succ = rtw->connect( ipaddr, port, doscene, doeye, dosound, doeyevid, dumpfile, reqwid, reqhei, eyewid, eyehei );

  fprintf(stdout, "Finished connect...\n");
  if( false == succ )
    {
      delete rtw;
      rtw=NULL;
    }

  return (void*)rtw;
}


void rteye_wrapper_uninit( void* myptr )
{
  rteye_wrapper* rtw = (rteye_wrapper*)(myptr);

  if( rtw )
    {
      delete rtw;
    }
}

void rteye_wrapper_update( void* myptr, long targ_msec )
{
  rteye_wrapper* rtw = (rteye_wrapper*)(myptr);
  
  if( rtw )
    {
      rtw->update( targ_msec );
    }
}

void rteye_wrapper_update_calib_pos( void* myptr, long targ_msec )
{
  rteye_wrapper* rtw = (rteye_wrapper*)(myptr);
  
  if( rtw )
    {
      rtw->update_calib_pos( targ_msec );
    }
}

cv::Mat rteye_wrapper_get_scene_frame( void* myptr )
{
  rteye_wrapper* rtw = (rteye_wrapper*)(myptr);
  
  if( rtw )
    {
      return rtw->get_scene_frame();
    }
  else
    {
      return cv::Mat();
    }
}


long rteye_wrapper_get_scene_frame_idx( void* myptr )
{
  rteye_wrapper* rtw = (rteye_wrapper*)(myptr);
  
  if( rtw )
    {
      return rtw->get_scene_frame_idx();
    }
  else
    {
      return -1000000000;
    }
}


long rteye_wrapper_get_scene_frame_pts( void* myptr )
{
  rteye_wrapper* rtw = (rteye_wrapper*)(myptr);
  
  if( rtw )
    {
      return rtw->get_scene_frame_pts();
    }
  else
    {
      return -1000000000;
    }
}


float rteye_wrapper_get_eyex( void* myptr )
{
  rteye_wrapper* rtw = (rteye_wrapper*)(myptr);
  
  if( rtw )
    {
      return rtw->get_eyex();
    }
  else
    {
      return -1000000000;
    }
}



float rteye_wrapper_get_eyey( void* myptr )
{
  rteye_wrapper* rtw = (rteye_wrapper*)(myptr);
  
  if( rtw )
    {
      return rtw->get_eyey();
    }
  else
    {
      return -1000000000;
    }
}


void rteye_wrapper_calibrate( void* myptr )
{
  rteye_wrapper* rtw = (rteye_wrapper*)(myptr);
  
  if( rtw )
    {
      if( rtw->rtc->calibrating )
	{
	  fprintf(stderr, "WTF rteye_wrapper_calibrate  -- you are attempting to double-calibrate while it is already calibrating?\n");
	}
      else
	{
	  rtw->calibrate();
	}
    }
}


float rteye_wrapper_get_calibx( void* myptr )
{
  rteye_wrapper* rtw = (rteye_wrapper*)(myptr);
  
  if( rtw )
    {
      return rtw->get_calib_x();
    }
  else
    {
      return -10000000;
    }
}


float rteye_wrapper_get_caliby( void* myptr )
{
  rteye_wrapper* rtw = (rteye_wrapper*)(myptr);
  
  if( rtw )
    {
      return rtw->get_calib_y();
    }
  else
    {
      return -10000000;
    }
}

void rteye_wrapper_endcalib( void* myptr )
{
  rteye_wrapper* rtw = (rteye_wrapper*)(myptr);
  
  if( rtw )
    {
      rtw->endcalib();
    }
}
