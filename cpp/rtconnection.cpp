
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

 


rtconnection::~rtconnection()
{
  { fprintf(stderr, "CALLING RTCONNECTION DESTRUCTOR METHOD!!!\n"); }
}




rtconnection::rtconnection()
{
  loop = NULL;
  calibrated=false;
  calibrating=false;
  zeroed_pts=0;
  fromfile=false;
  scenevid=false;
  eyetracking=false;
  audio=false;
  eyevid=false;
}
  
rtconnection::rtconnection(loop_condition* _loop)
{
  loop = _loop;
  calibrated=false;
  calibrating=false;
  zeroed_pts=0;
  fromfile=false;
  scenevid=false;
  eyetracking=false;
  audio=false;
  eyevid=false;
}

