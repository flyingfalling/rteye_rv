
 

















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






#define THREAD_WAKEUP_MS 5




void kill_stream_loop( const int& socket, const std::string message, loop_condition& loop )
{
  if( send( socket, message.c_str(), strlen(message.c_str()), 0 ) < 0 )
    {
      fprintf( stderr, "REV: error sending keepalive STOP message [%s]\n", message.c_str() );
      exit(1);
    }
}

void send_keepalive_loop( const int& socket, const std::string message, loop_condition& loop )
{
  long long SLEEP_TIME_MS=1000; 

  fprintf(stdout, "Starting keepalive loop msg [%s]\n", message.c_str() );

  rteye2::Timer t;
  while( true == loop )
    {
      
      t.reset();
      
      if( send( socket, message.c_str(), strlen(message.c_str()), 0 ) < 0 )
	{
	  fprintf( stderr, "REV: error sending keepalive message [%s]\n", message.c_str() );
	  exit(1);
	}

      double tel = t.elapsed();
      if( tel > 0.05 )
	{
	  fprintf(stderr, "WARNING, for keepalive [%s], sending keepalive message took [%s] msec!\n", tel*1000 );
	}
      
      
      
      
      if( true == loop )
	{
	  std::this_thread::sleep_for( std::chrono::milliseconds(SLEEP_TIME_MS) );
	}
      
    }
  fprintf(stdout, "Exiting keepalive loop [%s]\n", message.c_str());
  
}


int64_t SeekFunc(void* ptr, int64_t pos, int whence)
{
  return -1;
}

int ReadFunc(void* ptr, uint8_t* buf, int buf_size)
{
  gbuf* data = reinterpret_cast<gbuf*>(ptr);
  
#define MINSEND
#ifdef MINSEND

  size_t avail=data->avail();
  size_t nwaited=0;
#if DEBUG > 5
  rteye2::Timer t;
#endif

  
  
  while( (int)avail < buf_size && *(data->loop) )
    {
      ++nwaited;
      
      
      {
	std::unique_lock<std::mutex> lk( data->cv->mux );
	
	while( false == data->cv->val && true == *(data->loop) ) 
	  {
	    data->cv->cv.wait_for( lk, std::chrono::milliseconds( THREAD_WAKEUP_MS ) );
	  }

	avail = data->avail();
      }
    } 

#if DEBUG > 5
  if( nwaited > 0 )
    {
      fprintf( stdout, "REV: ReadFunc being called (for %d) (note curr avail in buf is %ld)(filled after %ld sleeps (%lf msec))\n", buf_size, data->avail(), nwaited, t.elapsed()*1000.0 );
    }
#endif
#else
  
#endif
  
  
  size_t bytesRead = data->pop_into( buf, buf_size );
  
  return bytesRead;
}

void copy_mats_a_to_b( const cv::Mat& a, cv::Mat& b )
{
  if( a.size() != b.size() || a.type() != b.type() )
    {
      fprintf(stderr, "REV: attempting to copy data from non-similar matrices A and B\n");
      exit(1);
    }
  
  a.copyTo(b);
}




template <typename T>
std::vector<T> json_as_vector(boost::property_tree::ptree const& pt, boost::property_tree::ptree::key_type const& key)
{
  std::vector<T> r;
  for (auto& item : pt.get_child(key))
    r.push_back(item.second.get_value<T>());
  return r;
}

boost::property_tree::ptree string2json( const std::string& s )
{
  boost::property_tree::ptree pt;
  std::istringstream is(s);
  boost::property_tree::read_json(is, pt);
  return pt;
}

std::string json2string( const boost::property_tree::ptree& pt )
{
  std::ostringstream buf; 
  boost::property_tree::write_json(buf, pt, false);
  return buf.str();
}

std::string map2json (const std::map<std::string, std::string>& map)
{
  boost::property_tree::ptree pt; 
  for (auto& entry: map) 
    {
      pt.put (entry.first, entry.second);
    }
  std::ostringstream buf; 
  boost::property_tree::write_json (buf, pt, false); 
  return buf.str();
}

void add_to_json( boost::property_tree::ptree& pt, const std::string& name, const std::string& value )
{
  pt.put( name, value );
}







template <typename T>
T get_single_json( const boost::property_tree::ptree& pt, const std::string& s )
{
  T ret;
  try
    {
      ret = pt.get<T>(s); 
      
    }
  catch( boost::property_tree::json_parser::json_parser_error )
    {
      fprintf(stderr, "Error getting single json [%s] from?\n", s.c_str());
      exit(1);
    }
  return ret;
  
}

template <typename T>
bool check_exists_json( const boost::property_tree::ptree& pt, const std::string& s )
{
  boost::property_tree::ptree::const_assoc_iterator it = pt.find(s);
  if( pt.not_found() == it )
    {
      return false;
    }
  else
    {
      return true;
    }
}


boost::property_tree::ptree get_request( const std::string& baseurl, const std::string& api_action )
{
  
  boost::property_tree::ptree response_data;
  
  
  
  
  auto const host = baseurl; 
  auto const port = "80";
  auto const target = api_action;
  boost::asio::io_context ioc;

  boost::asio::ip::tcp::resolver resolver{ioc};
  boost::asio::ip::tcp::socket socket{ioc};
  auto const results = resolver.resolve(host, port); 

  boost::asio::connect(socket, results.begin(), results.end() );

  int version = 11;
  
  
  boost::beast::http::request<boost::beast::http::string_body> req{boost::beast::http::verb::get, target, version};
  req.set(boost::beast::http::field::host, host);
  req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

  req.set(boost::beast::http::field::content_type, "application/json");
  
  
  req.prepare_payload();
  boost::beast::http::write( socket, req );

  boost::beast::flat_buffer buffer;
  boost::beast::http::response<boost::beast::http::dynamic_body> res;
  
  
  boost::beast::http::read(socket, buffer, res);
  
  
  
  
  
  std::stringstream mybody;
  mybody << boost::beast::buffers( res.body().data() );
  
  
  response_data = string2json( mybody.str() );
  
  
  
  return response_data;
}


boost::property_tree::ptree post_request( const std::string& baseurl, const std::string& api_action )
{
  
  boost::property_tree::ptree response_data;
  
  
  
  
  auto const host = baseurl; 
  auto const port = "80";
  auto const target = api_action;
  boost::asio::io_context ioc;
  boost::asio::ip::tcp::resolver resolver{ioc};
  boost::asio::ip::tcp::socket socket{ioc};
  auto const results = resolver.resolve(host, port); 

  boost::asio::connect(socket, results.begin(), results.end() );

  int version = 11;
  
  boost::beast::http::request<boost::beast::http::string_body> req{boost::beast::http::verb::post, target, version};
  req.set(boost::beast::http::field::host, host);
  req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

  req.set(boost::beast::http::field::content_type, "application/json");
  
  
  req.prepare_payload();
  boost::beast::http::write( socket, req );

  boost::beast::flat_buffer buffer;
  boost::beast::http::response<boost::beast::http::dynamic_body> res;
  

  boost::beast::http::read(socket, buffer, res);
  
  
  
  
  
  std::stringstream mybody;
  mybody << boost::beast::buffers( res.body().data() );
  
  
  response_data = string2json( mybody.str() );
  
  
  
  return response_data;
}


boost::property_tree::ptree post_request( const std::string& baseurl, const std::string& api_action, const boost::property_tree::ptree& data )
{

   
  boost::property_tree::ptree response_data;
  
  
  
  
  auto const host = baseurl; 
  auto const port = "80";
  auto const target = api_action;
  boost::asio::io_context ioc;
  boost::asio::ip::tcp::resolver resolver{ioc};
  boost::asio::ip::tcp::socket socket{ioc};
  auto const results = resolver.resolve(host, port); 

  boost::asio::connect(socket, results.begin(), results.end() );

  int version = 11;
  
  boost::beast::http::request<boost::beast::http::string_body> req{boost::beast::http::verb::post, target, version};
  req.set(boost::beast::http::field::host, host);
  req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

  req.set(boost::beast::http::field::content_type, "application/json");

  req.body() = json2string(data);
  
  
  req.prepare_payload();
  
  boost::beast::http::write( socket, req );

  boost::beast::flat_buffer buffer;
  boost::beast::http::response<boost::beast::http::dynamic_body> res;
  

  boost::beast::http::read(socket, buffer, res);
  
  
  
  
  
  std::stringstream mybody;
  mybody << boost::beast::buffers( res.body().data() );
  
  
  response_data = string2json( mybody.str() );
  
  
  
  return response_data;
}


std::string wait_for_status( const size_t& maxtimeouts, const std::string& baseurl, const std::string& api_action, const std::string& key, const std::vector<std::string>& values )
{
  
  
  
  
  bool running=true;

  size_t tried=0;
    
  
  
  std::string val="TOO_MANY_TRIES";
  
  
  
  while( running && tried <= maxtimeouts)
    {
      ++tried;
      
      
      
      
      
      

      
      auto const host = baseurl; 
      auto const port = "80";
      auto const target = api_action;
      boost::asio::io_context ioc;
      boost::asio::ip::tcp::resolver resolver{ioc};
      boost::asio::ip::tcp::socket socket{ioc};
      auto const results = resolver.resolve(host, port);
      boost::asio::connect(socket, results.begin(), results.end() );
      int version = 11;
      boost::beast::http::request<boost::beast::http::string_body> req{boost::beast::http::verb::get, target, version};
      req.set(boost::beast::http::field::host, host);
      req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

      req.set(boost::beast::http::field::content_type, "application/json");

      req.prepare_payload();
      
      
      
  
      boost::beast::http::write( socket, req );

      boost::beast::flat_buffer buffer;
      boost::beast::http::response<boost::beast::http::dynamic_body> res;
      

      boost::beast::http::read(socket, buffer, res);

      
      
      
      
      
      
      if( res.result() == boost::beast::http::status::ok )
      
      
	{
	  std::stringstream mybody;
	  mybody << boost::beast::buffers( res.body().data() );
	  
	  boost::property_tree::ptree response_data = string2json( mybody.str() );
	  
	  val = get_single_json<std::string>( response_data, key );
	  
	  for(size_t x=0; x<values.size(); ++x)
	    {
	      
	      if( is_same_string( values[x], val ) )
		{
		  running = false;
		}
	    }
	}
      if(running)
	{
	  
	  
	  std::this_thread::sleep_for( std::chrono::milliseconds(500) );
	}
    }

  fprintf(stdout, "Returning [%s] after [%ld] of [%ld] tries\n", val.c_str(), tried, maxtimeouts );
  return val;
}

bool propertyexists( const boost::property_tree::ptree& pt, const std::string& targprop, const std::string& origstring, const bool& erroronfail )
{
  boost::property_tree::ptree::const_assoc_iterator it = pt.find( targprop );
  if( pt.not_found() == it )
    {
      if( true == erroronfail )
	{
	  fprintf(stdout, "Couldn't find property [%s]!\n\n%s\n\n", targprop.c_str(), origstring.c_str());
	  exit(1);
	}
      else
	{
	  return false;
	}
    }
  return true;
}



void json_to_data_pkg_buffer( const std::string& s, data_pkg_buffer& dpb )
{
  std::istringstream iss( s );

  boost::property_tree::ptree pt;
  
  try
    {
      boost::property_tree::json_parser::read_json(iss, pt);
    }
  catch( boost::property_tree::json_parser::json_parser_error )
    {
      fprintf(stderr, "REV: Threw an error from string [%s]\n", s.c_str());
      return;
    }

  
  bool marker2dexists = propertyexists( pt, "marker2d", s, false );
  if( marker2dexists )
    {
      fprintf(stdout, "****************** REV: calibrating, marker2d exists! ******************\n" );
    }
  
  propertyexists( pt, "s", s, true );
  propertyexists( pt, "ts", s, true );
  
  int status = pt.get<int>("s");
  
  if( 0 != status )
    {
      
      
      return;
    }

  

  long long ts = pt.get<long long>("ts");
  
  bool gidxexists = propertyexists( pt, "gidx", s );
  if( false == gidxexists ) 
    {
      
      bool ptsexists = propertyexists( pt, "pts", s );
      if( false == ptsexists )
	{
	  
	  if( propertyexists( pt, "marker2d", s ) )
	    {
	      std::vector<float> marker2d = get_vector_json<float>(pt, "marker2d");
	      
	      marker2d_pkg marker2dpkg( ts, marker2d );
	      dpb.add_marker2d( marker2dpkg );
	    }
	}
      else
	{
	  
	  
	  
	  long long pts = pt.get<long long>("pts");
	  bool pvexists = propertyexists( pt, "pv", s );
	  int pv = pt.get<int>("pv");
	  if( true == pvexists )
	    {
	      
	      pts_sync_pkg myptssyncpkg( ts, pts, pv );
	      
	      
	      dpb.sync_pkg( myptssyncpkg );
	      
	    }
	  else
	    {
	      
	    }
	}

    }
  else 
    {
      long gidx = pt.get<long>("gidx");

      
      
      bool eyeexists = propertyexists( pt, "eye", s );
      if( false == eyeexists )
	{
	  
	  
	  if( true == propertyexists( pt, "gp", s ) )
	    {
	      
	      std::vector<float> gp = get_vector_json<float>(pt, "gp");
	      if( false == propertyexists( pt, "l", s ) )
		{
		  fprintf(stderr, "REV: error gp should include l, latency\n");
		  exit(1);
		}
	      long latency = pt.get<long>( "l" );
	      gp_pkg mygppkg( ts, gidx, latency, gp );
	      
	      
	      
	      dpb.add_gp( mygppkg );
	      
	    }
	  else
	    {
#ifdef GP3
	      
	      if( false == propertyexists( pt, "gp3" , s ) )
		{
		  fprintf(stderr, "REV: error gp3 should include gp3\n");
		  exit(1);
		  
		}

	      if( true == propertyexists( pt, "l", s ) )
		{
		  fprintf(stderr, "REV: l (latency) should not exist in gp3 according to docs...\n");
		  exit(1);
		}

	      std::vector<float> gp3 = get_vector_json<float>(pt, "gp3");
	      gp3_pkg mygp3pkg( ts, gidx, gp3 );
	      dpb.gp3.push_back(mygp3pkg);
#endif
	    }
	} 
      else
	{
	  
	}
    }
  
}


std::string tobii_REST::create_tobii_project()
{
  boost::property_tree::ptree ret = post_request( base_url, "/api/projects" );
  return get_single_json<std::string>( ret, "pr_id" );
}

std::string tobii_REST::create_tobii_participant( const std::string& proj_id )
{
  boost::property_tree::ptree tmp;
  add_to_json(tmp, "pa_project", proj_id );
  boost::property_tree::ptree ret = post_request( base_url, "/api/participants", tmp );
  return get_single_json<std::string>( ret, "pa_id" );
}

std::string tobii_REST::create_tobii_calibration( const std::string& proj_id, const std::string& partic_id )
{
  boost::property_tree::ptree tmp;
  add_to_json(tmp, "ca_project", proj_id );
  add_to_json(tmp, "ca_type", "default" );
  add_to_json(tmp, "ca_participant", partic_id );
  boost::property_tree::ptree ret = post_request( base_url, "/api/calibrations", tmp );
  return get_single_json<std::string>( ret, "ca_id" );
}

void tobii_REST::start_tobii_calibration( const std::string& calib_id)
{
  boost::property_tree::ptree ret = post_request( base_url, "/api/calibrations/" + calib_id + "/start" );
}


bool is_same_string( const std::string& s1, const std::string& s2 )
{
  return ( 0 == s1.compare(s2) );
}


void draw_calib_pos(cv::Mat& mymat, const float& eyex, const float& eyey)
{
  cv::Scalar color(0,0,255);
  
  
  
  
  
  size_t wid = mymat.size().width;
  size_t hei = mymat.size().height;

  int radius = wid * 0.023333;
  int thickness= wid * 0.0063333 ; 
    
  int eyexpix = wid * eyex; 
  int eyeypix = hei * eyey;

  
  cv::circle( mymat, cv::Point( eyexpix, eyeypix ), radius, color, thickness );
}


void draw_eye_pos(cv::Mat& mymat, const float& eyex, const float& eyey)
{
  
  cv::Scalar color(0,255,255);
  
  
  
  
  
  size_t wid = mymat.size().width;
  size_t hei = mymat.size().height;

  int radius = wid * 0.033333;
  int thickness= wid * 0.0033333 ; 
    
  int eyexpix = wid * eyex; 
  int eyeypix = hei * eyey;

  
  cv::circle( mymat, cv::Point( eyexpix, eyeypix ), radius, color, thickness );
}

