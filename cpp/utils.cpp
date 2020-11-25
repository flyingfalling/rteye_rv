


#include <mutex>
#include <map>

#include <cstdlib>
#include <cstdio>
 #include <sys/types.h>
 #include <sys/stat.h>
#include <pwd.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif  

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





bool copyFile(const std::string& SRC, const std::string& DEST)
{
  std::ifstream src(SRC, std::ios::binary);
  std::ofstream dest(DEST, std::ios::binary);
  dest << src.rdbuf();
  return src && dest;
}








std::string replace_illegals( const std::string& s )
{
  std::string rs=s;
  std::string illegalChars = "\\/:?\"<>|(), ";
  for (auto it = rs.begin() ; it != rs.end() ; ++it)
    {
      bool found = (illegalChars.find(*it) != std::string::npos);
      if(found)
	{
	  *it = '_';
	}
    }
  return rs;
}
bool samestring( const std::string& a, const std::string& b )
{
  if(a.compare(b) == 0 ) {return true;}
  else { return false; }
}


int do_mkdir(const char *path, mode_t mode)
{
    struct stat            st;
    int             status = 0;

    if (stat(path, &st) != 0)
    {
       
      if (mkdir(path, mode) != 0 && errno != EEXIST)
	{ status = -1; }
    }
    else if (!S_ISDIR(st.st_mode))
    {
        errno = ENOTDIR;
        status = -1;
    }

    return(status);
}

 
int mkpath(const std::string& path, mode_t mode)
{
    char           *pp;
    char           *sp;
    int             status;
    std::string    tmpstr = path;
    char           *copypath = (char*)tmpstr.c_str();

    status = 0;
    pp = copypath;
    while (status == 0 && (sp = strchr(pp, '/')) != 0)
    {
        if (sp != pp)
        {
             
            *sp = '\0';
            status = do_mkdir(copypath, mode);
            *sp = '/';
        }
        pp = sp + 1;
    }
    if (status == 0)
      status = do_mkdir(path.c_str(), mode);
    
    return (status);
}


void add_timing( const int64_t& msec, cv::Mat& m)
{
  int width = m.size().width;
  int height= m.size().height;
  std::string toput = std::to_string( msec ) + " msec";
  float scale = width/640.0;
  
  cv::putText( m, toput, cv::Point(10*scale, 30*scale), cv::FONT_HERSHEY_PLAIN, 2.0*scale, cv::Scalar(150,255,255) );
}

void add_name( const std::string& name, cv::Mat& m)
{
  int width = m.size().width;
  int height= m.size().height;
  
  float scale = width/640.0;
  
  cv::putText( m, name, cv::Point(10*scale, 2*30*scale), cv::FONT_HERSHEY_PLAIN, 1.25*scale, cv::Scalar(150,255,255) );
}


std::string Env::getUserName()
{
  struct passwd *pw;
  uid_t uid;
  
  uid = geteuid ();
  pw = getpwuid (uid);
  if(pw)
    {
      return std::string(pw->pw_name);
    }
  return std::string("");
}
