



#pragma once

#include <socket_helpers.hpp>
#include <string_manip.hpp>




struct recvbuffer;

struct data_pkg_buffer;


void draw_eye_pos(cv::Mat& mymat, const float& eyex, const float& eyey);
void draw_calib_pos(cv::Mat& mymat, const float& eyex, const float& eyey);


long ts_to_pts( const long& ts );
void json_to_data_pkg_buffer( const std::string& s, data_pkg_buffer& dpb );

template <typename T>
std::vector<T> json_as_vector(boost::property_tree::ptree const& pt, boost::property_tree::ptree::key_type const& key);

template <typename T>
std::vector<T> get_vector_json( boost::property_tree::ptree const& pt, const std::string& s)
{
  std::vector<float> ret;
  
  for (T i : json_as_vector<T>(pt, s) )
    {
      ret.push_back( i );
    }
  return ret;
}

bool propertyexists( const boost::property_tree::ptree& pt, const std::string& targprop, const std::string& origstring, const bool& erroronfail=false );

boost::property_tree::ptree string2json( const std::string& s );

std::string json2string( const boost::property_tree::ptree& pt );

std::string map2json (const std::map<std::string, std::string>& map);

void add_to_json( boost::property_tree::ptree& pt, const std::string& name, const std::string& value );

template <typename T>
T get_single_json( const  boost::property_tree::ptree& pt, const std::string& s );

template <typename T>
bool check_exists_json( const boost::property_tree::ptree& pt, const std::string& s );

boost::property_tree::ptree post_request( const std::string& baseurl, const std::string& api_action, const boost::property_tree::ptree& data );

boost::property_tree::ptree post_request( const std::string& baseurl, const std::string& api_action );

std::string wait_for_status( const size_t& maxtimeouts, const std::string& baseurl, const std::string& api_action, const std::string& key, const std::vector<std::string>& values );

int ReadFunc(void* ptr, uint8_t* buf, int buf_size);

int64_t SeekFunc(void* ptr, int64_t pos, int whence);

void copy_mats_a_to_b( const cv::Mat& a, cv::Mat& b );

bool is_same_string( const std::string& s1, const std::string& s2 );

void send_keepalive_loop( const int& socket, const std::string message, loop_condition& loop );

void kill_stream_loop( const int& socket, const std::string message, loop_condition& loop );












#ifdef USE_APPLE_VIDTOOLBOX
static enum AVPixelFormat negotiate_pixel_format_ios_vidtoolbox(struct AVCodecContext *s, const enum AVPixelFormat *fmt)
{
  while (*fmt != AV_PIX_FMT_NONE)
    {
      if (*fmt == AV_PIX_FMT_VIDEOTOOLBOX)
	{  
	  if (s->hwaccel_context == NULL)
	    {
	      int result = av_videotoolbox_default_init(s);
	      if (result < 0)
		{
		  
		  return s->pix_fmt;
		}
	    }
	  
	  return *fmt;
	}
      ++fmt;
    }
  
  return s->pix_fmt;
}
#endif







struct tobii_REST
{
  std::string base_url;

  tobii_REST( const std::string& burl ) : base_url(burl) {   }
  
  std::string create_tobii_project();
  
  std::string create_tobii_participant( const std::string& proj_id );
  
  std::string create_tobii_calibration( const std::string& proj_id, const std::string& partic_id );
  
  void start_tobii_calibration( const std::string& calib_id);
};


struct bool_condvar
{
  std::condition_variable cv;
  std::mutex mux; 
  bool val;

  bool_condvar()
  {
    val=false;
  }
};

struct int64_condvar
{
  std::condition_variable cv;
  std::mutex mux; 
  int64_t val;
  int64_condvar()
  {
    val=0;
  }
};



struct gbuf
{
  std::mutex mux;
  std::vector<uint8_t> dat;
  size_t currsize;
  size_t max_size;
  size_t head;
  size_t tail;
  
  std::vector<uint8_t> tmp;
  
  bool_condvar* cv;
  loop_condition* loop;

  void init( const size_t& _maxsize = 10000000 )
  {
    max_size = _maxsize;
    head=0;
    tail=0;
    currsize=0;
    dat.resize(max_size);
    cv=NULL;
    loop=NULL;
  }
  
  gbuf(const size_t& _maxsize = 10000000)
    : max_size( _maxsize )
  {
    head=0;
    tail=0;
    currsize=0;

    cv=NULL;
    loop=NULL;
    dat.resize(max_size);
  }
    
  gbuf( bool_condvar*& _cv, const size_t& _maxsize = 10000000 ) 
    : max_size( _maxsize )
  {
    head=0;
    tail=0;
    currsize=0;

    loop=NULL;
    cv = _cv;
    dat.resize(max_size);
  }
  
  void lock()
  {
    mux.lock();
  }
  
  void unlock()
  {
    mux.unlock();
  }

  void add( const std::vector<uint8_t>& toadd )
  {
    if( toadd.size() > dat.size() )
      {
	fprintf(stderr, "Trying to write data to buf than possible\n");
	exit(1);
      }
    
    
    lock();
    if( tail + toadd.size() > dat.size() )
      {
	size_t diff = tail+toadd.size() - dat.size();
	std::copy(  std::begin( toadd ), std::begin(toadd)+(toadd.size()-diff), std::begin(dat)+tail );
	std::copy(  std::begin(toadd)+(toadd.size()-diff), std::end(toadd), std::begin(dat) );
	tail = diff;
	
      }
    else
      {
	std::copy(  std::begin( toadd ), std::end( toadd ), std::begin(dat)+tail );
	tail+=toadd.size();
	
      }
    
    currsize += toadd.size();
    if(currsize > dat.size() )
      {
	fprintf(stderr, "REV: error, we are over ring buffer size!?!\n");
	exit(1);
      }
    
    unlock();
  }
  
  void add_nomux( const std::vector<uint8_t>& toadd, const size_t addsize )
  {
    if( addsize > dat.size() )
      {
	fprintf(stderr, "Trying to write data to buf than possible\n");
	exit(1);
      }
    
    if( tail + addsize > dat.size() )
      {
	size_t diff = tail+addsize - dat.size();
	std::copy(  std::begin( toadd ), std::begin(toadd)+(addsize-diff), std::begin(dat)+tail );
	std::copy(  std::begin(toadd)+(addsize-diff), std::begin(toadd)+addsize, std::begin(dat) );
	tail = diff;
	
      }
    else
      {
	std::copy(  std::begin( toadd ), std::begin( toadd )+addsize, std::begin(dat)+tail );
	tail+=addsize;
	
      }
    
    currsize += addsize;
    if(currsize > dat.size() )
      {
	fprintf(stderr, "REV: error, we are over ring buffer size!?!\n");
	exit(1);
      }
    
  }

  void add( const std::vector<uint8_t>& toadd, const size_t addsize )
  {
    lock();
    if( addsize > dat.size() )
      {
	fprintf(stderr, "Trying to write data to buf than possible\n");
	exit(1);
      }
    
    if( tail + addsize > dat.size() )
      {
	size_t diff = tail+addsize - dat.size();
	std::copy(  std::begin( toadd ), std::begin(toadd)+(addsize-diff), std::begin(dat)+tail );
	std::copy(  std::begin(toadd)+(addsize-diff), std::begin(toadd)+addsize, std::begin(dat) );
	tail = diff;
	
      }
    else
      {
	std::copy(  std::begin( toadd ), std::begin( toadd )+addsize, std::begin(dat)+tail );
	tail+=addsize;
	
      }
    
    currsize += addsize;
    if(currsize > dat.size() )
      {
	fprintf(stderr, "REV: error, we are over ring buffer size!?!\n");
	exit(1);
      }
    unlock();
  }
  
  size_t avail()
  {
    size_t a=0;
    lock();
    a = currsize;
    unlock();
    return a;
  }
  
  size_t read_into( uint8_t* dst, const size_t& ntoread )
  {
    size_t ncopied= currsize >= ntoread ? ntoread : currsize ;

    lock();
    if( ncopied > dat.size() )
      {
	fprintf(stderr, "REV: in buf, pop_into requested larger than buf size\n");
	exit(1);
      }
    
    if( head + ncopied > dat.size() )
      {
	fprintf(stdout, "Reading over straddled GBUF\n");
	size_t diff = (head + ncopied) - dat.size();
	std::memcpy( dst, dat.data()+head, (ncopied-diff) );
	std::memcpy( dst+(ncopied-diff), dat.data()+0, diff );
      }
    else
      {
	std::memcpy( dst, dat.data()+head, ncopied );
      }
    unlock();
    return ncopied;
  }
  
  size_t pop_into( uint8_t* dst, const size_t& ntoread )
  {
    size_t ncopied = currsize >= ntoread ? ntoread : currsize ;
    lock();
    if( ncopied > dat.size() )
      {
	fprintf(stderr, "REV: in buf, pop_into requested larger than buf size\n");
      }
    
    if( head + ncopied > dat.size() )
      {
	size_t diff = head + ncopied - dat.size();
	std::memcpy( dst, dat.data()+head, (ncopied-diff) );
	std::memcpy( dst+(ncopied-diff), dat.data()+0, diff );
	head = diff;
      }
    else
      {
	std::memcpy( dst, dat.data()+head, ncopied );
	head += ncopied;
      }
    
    currsize -= ncopied;
    
    unlock();
    
    return ncopied;
  }
};






struct buffered_socket
{
  size_t mesg_size;
  recvbuffer buf;
  size_t triggersize;
  int socket;
  
  buffered_socket()
  {
    mesg_size=4096;
  }
  
  buffered_socket( const int& _socket, const size_t& _triggersize, const size_t& _mesgsize )
  {
    init( _socket, _triggersize, _mesgsize );
  }

  void init( const int& _socket, const size_t& _triggersize, const size_t& _mesgsize )
  {
    size_t size = _triggersize + _mesgsize; 
    buf.init( size ); 
    if( buf.buf.size() !=  size )
      {
	fprintf(stderr, "REV: buffered_socket constructor, buf didn't get reallocated (copy constructor?)\n");
	exit(1);
      }
    mesg_size = _mesgsize;
    triggersize = _triggersize;
    socket = _socket;
  }

  
  
  size_t avail()
  {
    return buf.avail();
  }
  
};










template <typename T>
struct rotating_timed_buffer
{
  std::mutex mux;
  uint32_t head;
  uint32_t stored;
  std::vector<T> items;

  void lock()
  {
    mux.lock();
  }

  void unlock()
  {
    mux.unlock();
  }
  
  void init( const size_t& cap )
  {
    stored = 0;
    items.resize( cap );
    head = capacity()-1;
  }
  
  uint64_t capacity()
  {
    return items.size();
  }

  rotating_timed_buffer()
  {
    head=0;
    stored=0;
  }
  
  rotating_timed_buffer( const uint32_t& _capacity )
  {
    head=0;
    stored=0;
    init( _capacity );
  }

  int32_t getidx( const int32_t& idx )
  {
    int32_t addbuf = 0;
    if( idx < 0 )
      {
	addbuf = capacity(); 
      }

    if( addbuf < 0 )
      {
	fprintf(stderr, "REV: wtf addbuf<0?\n");
      }
    
    int32_t idx2 = ( head + idx + addbuf ) % capacity();
    if( idx2 < 0 || idx2 >= capacity() )
      {
	fprintf(stderr, "REV: error, getidx fails (getidx==[%ld], cap=[%ld]\n", idx2, capacity());
	exit(1);
      }
    
    return idx2;
  }
  
  
  void add( const T& item )
  {
    int32_t underlying_head = getidx( 0 );
    items[underlying_head] = item;
    head = getidx( -1 );
    stored += 1;
    
    if( stored > capacity() )
      {
	stored = capacity();
      }
  }
  
  
  bool get_nth_underlying( const int32_t& nth, T& item )
  {
    if( nth < 0 || nth >= capacity() )
      {
	fprintf(stderr, "REV: error attempting to get illegal nth underlying\n");
	exit(1);
      }
    else
      {
	item = items[ nth ];
      }
  }

  
  bool get_nth_from_head( const int32_t& nth, T& item )
  {
    
    if( nth > stored )
      {
	return false; 
      }
    else
      {
	int32_t underlying = getidx( nth );
	item = items[ underlying ];
	return true;
      }
  }


  
  
  
  
  
  
  
  int32_t get_straddled( const int64_t pts, T& item, int64_t allowed_error=0 )
  {
    
    
    if( stored > 0 )
      {
	
	int32_t newest = newest_idx_less_than_equal_to( pts ); 
    
	
	
    
	if( newest > 0 )
	  {
	    
	    
	    if( 1 == newest )
	      {
		
		if( items[getidx( newest )].get_time() + allowed_error > pts )
		  {
		    item = items[getidx( newest )];
		    allowed_error = newest_minus_target( pts );
		    return newest;
		  }
		
		else
		  {
		    item = items[getidx( newest )];
		    allowed_error = newest_minus_target( pts );
		    return 0; 
		  }
	      }
	    else 
	      {
		item = items[getidx( newest )];
		allowed_error = newest_minus_target( pts );
		return newest;
	      }
	  }
	else 
	  {
	    
	    item = items[getidx( stored )];
	    allowed_error = oldest_minus_target( pts ); 
	    return -1; 
	  }
      }
    else
      {
	return -1; 
      }
  }
  
  
  int32_t get_newest( T& item )
  {
    
    
    if( stored > 0 )
      {
	item = items[ getidx(1) ];
	return 1;
      }
    else
      {
	return -1; 
      }
  }

  
  int32_t get_oldest( T& item )
  {
    
    
    if( stored > 0 )
      {
	int32_t tail = getidx( stored );
	item = items[tail];
	
      }
    return stored;
  }

  bool consume_oldest( )
  {
    if( stored > 0 )
      {
	stored -= 1;
	return true;
      }
    else
      {
	return false;
      }
  }

  
  int64_t oldest_minus_target( const int64_t& pts ) 
  {
    
    if( stored > 0 )
      {
	int64_t oldest_time = items[ getidx(stored) ].get_time(); 
	return (oldest_time - pts); 
      }
    else
      {
	return 0;
      }
  }

  
  
  
  int64_t newest_minus_target( const int64_t& pts ) 
  {
    
    if( stored > 0 )
      {
	int64_t newest_time = items[ getidx(1) ].get_time(); 
	return (newest_time - pts);
      }
    else
      {
	return 0;
      }
  }

  int32_t oldest_idx_greater_than( const int64_t& pts )
  {
    if(  stored > 0 )
      {
	    
    int32_t idx = stored;

    while( idx > 0
	   && items[getidx(idx)].get_time() <= pts )
      {
	--idx;
      }
    
    
    if( items[getidx(idx)].get_time() > pts )
      {
	return idx;
      }
    else
      {
	return 0;
      }
      }
    else
      {
	return 0;
      }
  }

  
  
  int32_t newest_idx_less_than_equal_to( const int64_t& pts )
  {
    if(stored>0 )
      {
	
    
	int32_t idx = 1;

	while( idx < stored
	       && items[getidx(idx)].get_time() > pts )
	  {
	    ++idx;
	  }
    
	

	if( items[getidx(idx)].get_time() <= pts )
	  {
	    return idx;
	  }
	else
	  {
	    return 0;
	  }
      }
    else
      {
	return 0;
      }
  }
  
  
  
  
  
  
  
  
  int32_t flush_all_less_than_equal_to( const int64_t& pts )
  {
    int32_t idx = oldest_idx_greater_than( pts );

    stored = idx;
    
    return stored;
  }

  int32_t flush_less_than_idx( const int32_t& idx )
  {
    if( idx > 0 && idx < stored )
      {
	stored = idx+1;
	return stored;
      }
    else
      {
	return 0;
      }
  }
  
  
  
  
  
  
  
  
  int32_t flush_all_less_than( const int64_t& pts )
  {
    int32_t idx = newest_idx_less_than_equal_to( pts );
    
    stored = idx;
    
    return stored;
  }

  
  
  
  
  
  int32_t get_oldest_greater_than( const int64_t pts, T& item, const int64_t& allowed_error=0 )
  {
    if( stored > 0 )
      {
    int32_t oldest = oldest_idx_greater_than( pts + allowed_error );
    if( oldest > 0 )
      {
	item = items[ getidx( oldest ) ];
      }
    return oldest;
      }
    else
      {
	return 0;
      }
      
  }
  
  
};




struct mpegts_parser
{
  
  bool isinit;
  
  int dstX;
  int dstY;
  
  size_t MIN_THRESH;
  
  
  AVFrame* tmp_decoded_aud_frame;
  AVFrame* decoded_aud_frame;

  std::mutex tmp_decoded_frame_buffer_mux;
  std::queue<AVFrame*> tmp_decoded_frame_buffer;
  
  
  AVFrame* decoded_frame;
  AVFrame* decoded_frameBGR;
  
  rotating_timed_buffer< timed_mat > rmmb;
  rotating_timed_buffer< timed_audio_frame > rafb;

  timed_mat _mm;
  cv::Mat decoded_BGR_mat;
  mat_timing mt;
    
  AVCodec* vid_codec;
  AVCodecContext* vid_codec_context;
  
  AVCodec* aud_codec;
  AVCodecContext* aud_codec_context;

  gbuf mygbuf;
  std::vector<uint8_t> decoding_input_buffer; 
  int decoding_input_buffer_size;
  
  AVIOContext* input_output_context;
  AVFormatContext* format_context;
  
  int vid_stream_id;
  int aud_stream_id;
  
  SwsContext *swsContext;

  
  rteye2::Timer t;
  double meanframetime;
  double meanframetimetau;

  int64_t pts_timebase_hz_sec;
  int64_t audio_rate_hz_sec;
  int32_t audio_bytes_per_sample;
  
  mpegts_parser()
  {
    
    isinit=false;
    dstX=1920;
    dstY=0;
    MIN_THRESH=128*1024;

    tmp_decoded_aud_frame=NULL;
    decoded_aud_frame=NULL;
    decoded_frame=NULL;
    decoded_frameBGR=NULL;


    vid_codec = NULL;
    vid_codec_context = NULL;  
    aud_codec = NULL;
    aud_codec_context = NULL;

    input_output_context=NULL;
    format_context=NULL;
    vid_stream_id=-1;
    aud_stream_id=-1;
    swsContext = NULL;

    meanframetime=0;
    meanframetimetau=0;
    pts_timebase_hz_sec=-1;
    audio_rate_hz_sec=-1;
    audio_bytes_per_sample=-1;
    
    av_register_all();
    
    fprintf(stdout, "Done ctor of mpegts parser (i.e. register all)\n");
  }

  ~mpegts_parser()
  {
    fprintf(stdout, "Freeing mpegTS parser\n");
    

    if( isinit )
      {
	
	

	
	decoded_frameBGR->data[0] = NULL;
	
	av_frame_free(&decoded_frameBGR);
	fprintf(stdout, "Freed decoded BGR\n");
	av_frame_free(&decoded_frame);
	fprintf(stdout, "Freed decoded frame\n");
	
	av_frame_free(&tmp_decoded_aud_frame);
	av_frame_free(&decoded_aud_frame);
	while( tmp_decoded_frame_buffer.size() > 0 )
	  {
	    av_frame_free( & tmp_decoded_frame_buffer.front() );
	    tmp_decoded_frame_buffer.pop();
	  }
    
    
	
	
	return ;
	
	avcodec_free_context( &vid_codec_context );
	fprintf(stdout, "Freed vid ctx\n");

	avcodec_free_context( &aud_codec_context );
	fprintf(stdout, "Freed aud ctx\n");
    
	avformat_close_input( &format_context );
	fprintf(stdout, "Closed format ctx\n");
	
	

	av_freep(&input_output_context);
	fprintf(stdout, "freed IO\n");
      }
    
    
    
    
    
    
  }

  
  void init(  const size_t& framewid, const size_t& framehei, const size_t& vidbufsize, const size_t& audbufsize )
  {
    dstX = framewid;
    dstY = framehei;
    rmmb.init( vidbufsize );
    rafb.init( audbufsize );
  }
  
  
  bool init_decoding()
  {
    
    
    decoding_input_buffer_size = 32*1024;
    
    
    decoding_input_buffer.resize(  decoding_input_buffer_size + AV_INPUT_BUFFER_PADDING_SIZE );
    
    input_output_context = avio_alloc_context(decoding_input_buffer.data(),
					      decoding_input_buffer_size,  
					      
					      
					      0,                  
					      &mygbuf,          
					      &ReadFunc,     
					      NULL,              
					      
					      NULL ); 
    
    fprintf(stdout, "Finished alloc IO context\n");
    
    if( NULL == input_output_context )
      {
	fprintf(stderr, "Can't alloc Input/Output Context\n");
	exit(1);
      }

    
    format_context = avformat_alloc_context();
    if( NULL == format_context )
      {
	fprintf(stderr, "ERR, can't alloc avformat codec ctx\n");
	exit(1);
      }

    format_context->pb = input_output_context;

    int err;
    bool naive_file_open = true; 
    fprintf(stdout, "Will now do FMT context\n");
    if( naive_file_open )
      {
	fprintf(stdout, "Naive (automtic) probing for video/audio format\n");
	
	size_t ntries = 50;
	while( avformat_open_input( &format_context, NULL, NULL, NULL ) < 0 && ntries > 0)
	  {
	    fprintf(stderr, "REV: failed avformat_open_input (try %ld)\n", ntries);
	    std::this_thread::sleep_for( std::chrono::milliseconds(20) );
	    --ntries;
	  }
	if(ntries == 0 )
	  {
	    fprintf(stderr, "Final fail, avformat_open_input\n");
	    exit(1);
	  }
	
	err = avformat_find_stream_info( format_context, NULL );

	if( err < 0 )
	  {
	    fprintf(stderr, "REV: failed avformat_find_stream_info\n");
	    exit(1);
	  }

	av_dump_format( format_context, 0, NULL, 0 );
      }
    else
      {
	fprintf(stdout, "Manual probing for video/audio format\n");
	
	
	size_t probesize =  100*1024; 
	
	
	std::vector<uint8_t> tmpbuf( probesize + AVPROBE_PADDING_SIZE, 0 );

	size_t max_timeout_ms = 1000;
	size_t sleep_ms = 50;
	size_t waited=0;
	while( mygbuf.avail() < probesize )
	  {
	    fprintf(stdout, "Waiting to get enough data to probe video scene format [have %ld bytes out of %ld]\n", mygbuf.avail(), probesize);
	    std::this_thread::sleep_for( std::chrono::milliseconds(sleep_ms) );
	    waited += sleep_ms;
	    if( waited >= max_timeout_ms )
	      {
		fprintf(stderr, "REV: error, could not get enough video data from scene camera in time (waited %ld ms)\n", max_timeout_ms );
		return false;
	      }
	  }
	fprintf(stdout, "Got enough data...will now read into mygbuf...\n");
	
	
	mygbuf.pop_into( tmpbuf.data(), probesize );
	
	fprintf(stdout, "OK...finished reading into mybuf\n");
	
	AVProbeData pd = {0};
	
	pd.buf = tmpbuf.data();
	pd.buf_size = probesize;
	pd.filename = "";
	
	fprintf(stdout, "Will probe MPEG-TS data to determine format for parser/code/demux\n");
	
	int is_opened = 1;
	
	format_context->iformat = av_probe_input_format( &pd, is_opened );
	
	
	
	pd.buf = NULL;
	pd.buf_size = 0;
	
	
	
	fprintf(stdout, "Done probe. Set iformat\n");
	
	if(NULL == format_context->iformat->long_name)
	  {
	    fprintf(stdout, "huh, long name is null\n");
	  }
	
	fprintf(stdout, "Will print names...\n");
	fprintf(stdout, "Names [%s] [%s]\n", format_context->iformat->name, format_context->iformat->long_name );
	
	
	err = avformat_open_input(&format_context, NULL, NULL, NULL);
	
	if( err != 0 )
	  {
	    fprintf(stderr, "Error on avformat open input\n");
	    exit(1);
	    
	  }
	
	fprintf(stdout, "Finished opening the Format input\n");
	
	int ret = avformat_find_stream_info(format_context, NULL);

	fprintf(stdout, "Finished finding streams...\n");
	if (ret != 0)
	  {
	    fprintf(stderr, "Could not find stream information\n");
	    exit(1);
	  }
	
	av_dump_format( format_context, 0, NULL, 0);
	fprintf(stdout, "Finished dumping...\n");
    
      }
    
    
    
    fprintf(stdout, "Successfully probed format\n");
    
    
    {
      vid_stream_id = av_find_best_stream(format_context, AVMEDIA_TYPE_VIDEO,
					  -1, -1, &vid_codec, 0);

      fprintf(stdout, "Found best video stream [%d]\n", vid_stream_id );

      
      if( vid_stream_id < 0 )
	{
	  fprintf(stderr, "REV: couldn't find video stream\n");
	  exit(1);
	}

      if( NULL == vid_codec )
	{
	  fprintf(stderr, "Couldn't find codec for video stream from find_best_stream\n");
	  exit(1);
	}
    
            
      fprintf(stdout, "Trying to alloc video codec...\n");
      
      
      vid_codec = avcodec_find_decoder( format_context->streams[vid_stream_id]->codecpar->codec_id );
      vid_codec_context = avcodec_alloc_context3(vid_codec);

      
      int ret = avcodec_parameters_to_context(vid_codec_context, format_context->streams[vid_stream_id]->codecpar);

      

#ifdef USE_APPLE_VIDTOOLBOX
      bool USE_IOS_VIDTOOLBOX = false;
      if( USE_IOS_VIDTOOLBOX )
	{
	  
	  fprintf(stdout, " ---- SETTING GET_FORMAT to IOS VIDEO TOOLBOX\n");
	  vid_codec_context->get_format = negotiate_pixel_format_ios_vidtoolbox;
	}
#endif 
      
      fprintf(stdout, "Did params to context codec...\n");
      
      if(ret<0)
	{
	  fprintf(stderr, "REV: error, couldnt get params to context video...\n");
	  exit(1);
	}

      
      ret = avcodec_open2(vid_codec_context, vid_codec, NULL);
      if( ret < 0 )
	{
	  fprintf(stderr, "Error, couldn't open video codec context!\n");
	  exit(1);
	}


      pts_timebase_hz_sec = 90e3; 
      
      fprintf(stdout, "Finished opening (video) codec\n");
    }
    
    
    
    
    {
      fprintf(stdout, "Locating audio decoder context stream\n");
      aud_stream_id = av_find_best_stream(format_context, AVMEDIA_TYPE_AUDIO,
					  -1, -1, &aud_codec, 0);
      
      fprintf(stdout, "Found best audio stream [%d]\n", aud_stream_id );
      
      if( NULL == aud_codec )
	{
	  fprintf(stderr, "Couldn't find codec for audio stream from find_best_stream. Turning audio off (REV: maybe tobii version is <=1.14?)\n");
	}
      else
	{
	  if (aud_stream_id == -1)
	    {
	      fprintf(stderr, "REV: couldn't find audio stream\n");
	      exit(1);
	    }
	  
	  aud_codec = avcodec_find_decoder( format_context->streams[aud_stream_id]->codecpar->codec_id );
	  aud_codec_context = avcodec_alloc_context3(aud_codec);
	  int ret = avcodec_parameters_to_context(aud_codec_context, format_context->streams[aud_stream_id]->codecpar);
	  if( ret<0 )
	    {
	      fprintf(stderr, "REV: error, couldnt get params to context audio...\n");
	      exit(1);
	    }
      
	  ret = avcodec_open2(aud_codec_context, aud_codec, NULL);
	  if( ret < 0 )
	    {
	      fprintf(stderr, "Error, couldn't open aud codec context!\n");
	      exit(1);
	    }
	  audio_rate_hz_sec = aud_codec_context->sample_rate; 
	  audio_bytes_per_sample = av_get_bytes_per_sample(aud_codec_context->sample_fmt);
	}

      fprintf(stdout, "Finished init audio codec...\n");
    } 
    
    
    
    tmp_decoded_frame_buffer_mux.lock();
    tmp_decoded_frame_buffer.push( av_frame_alloc() ); 

    if (!tmp_decoded_frame_buffer.back() || tmp_decoded_frame_buffer.size() != 1 )  
      {
	fprintf(stderr, "Could not allocate tmpdecoded video frame (av_frame_alloc) (or not size == 1?)\n");
	exit(1);
      }
    tmp_decoded_frame_buffer_mux.unlock();

    tmp_decoded_aud_frame = av_frame_alloc();

    
    if (!tmp_decoded_aud_frame) 
      {
	fprintf(stderr, "Could not allocate tmpdecoded audio frame (av_frame_alloc)\n");
	exit(1);
      }

    decoded_frameBGR = av_frame_alloc(); 
    if (!decoded_frameBGR) 
      {
	fprintf(stderr, "Could not allocate decoded video frame BGR (av_frame_alloc)\n");
	exit(1);
      }
    
    av_frame_unref(tmp_decoded_aud_frame);
    
    tmp_decoded_frame_buffer_mux.lock();
    av_frame_unref(tmp_decoded_frame_buffer.front());
    tmp_decoded_frame_buffer_mux.unlock();

    av_frame_unref(decoded_frameBGR);
    
    fprintf(stdout, "=========================== Finished mpegts_parser init_decoding()\n");
    
    isinit = true;

    return true;
  }  


  
  
  void make_av_frame_BGR( const int& _dstX, const int& _dstY )
  {
    
    if( NULL == swsContext )
      {
	init_BGR_converter( _dstX, _dstY );
      }
    
    
    
    sws_scale(swsContext, decoded_frame->data, decoded_frame->linesize, 0, decoded_frame->height, decoded_frameBGR->data, decoded_frameBGR->linesize);
    
    
    mt.copy_timing(decoded_frame);
  } 

    
  void init_BGR_converter(const int& _dstX=640, const int& _dstY=320)
  {
    dstX = _dstX;
    dstY = _dstY;
    

    
    int srcX = decoded_frame->width;
    int srcY = decoded_frame->height;
    fprintf(stdout, "REV: Initial decoded BGR image size is %d %d\n", srcX, srcY );
    if( srcX < 1 || srcY < 1 )
      {
	fprintf(stderr, "REV: error init BGR convertor, srcX or srcY is less than or equal to zero\n");
	exit(1);
      }
    if( 0 == dstY && 0 == dstX )
      {
	fprintf(stderr, "REV: error init dstX and dstY are both 0\n");
	exit(1);
      }
    if( 0 == dstY )
      {
	fprintf(stdout, "dstY (requested video frame size height) is 0, so Rescaling based on decoded frame dimensions (ratio) and requested width\n");
	double scaling = (double)dstX / srcX; 
	dstY = (srcY * scaling); 
      }
    else if( 0 == dstX )
      {
	fprintf(stdout, "dstX (requested video frame size width) is 0, so Rescaling based on decoded frame dimensions (ratio) and requested height\n");
	double scaling = (double)dstY / srcY; 
	dstX = (srcX * scaling); 
      }
        
    
    fprintf(stdout, "INITIALIZING: Setting SWS scaling context to go from original w: %d h: %d to new w: %d h: %d\n", srcX, srcY, dstX, dstY);

    
    
    
    enum AVPixelFormat src_pixfmt = (enum AVPixelFormat)decoded_frame->format;
    

    if( swsContext )
      {
	sws_freeContext( swsContext );
      }

    
    fprintf(stdout, "REV: pixel format of incoming frames is: [%s] Size: (W: %ld H: %ld)\n", av_get_pix_fmt_name( src_pixfmt ), srcX, srcY );
    

    
    
    
    swsContext = sws_getContext(srcX, srcY, src_pixfmt, dstX, dstY, AV_PIX_FMT_BGR24, SWS_AREA, NULL, NULL, NULL);
    
    fprintf(stdout, "Alloc decoded BGR to %d %d\n", dstX, dstY);
    decoded_BGR_mat = cv::Mat::zeros( dstY, dstX, CV_8UC3 );
    
     
    int linesize_align=32;
    int ret = av_image_alloc( decoded_frameBGR->data, decoded_frameBGR->linesize,
			      dstX, dstY, AV_PIX_FMT_BGR24, linesize_align );

    if( !ret )
      {
	fprintf(stderr, "Couldnt alloc new image for BGR conversion\n");
	exit(1);
      }
    
    decoded_frameBGR->data[0] = (uint8_t*)(decoded_BGR_mat.data);

    
    decoded_frameBGR->width = dstX;
    decoded_frameBGR->height = dstY;

    
    av_image_fill_arrays( decoded_frameBGR->data, decoded_frameBGR->linesize, (uint8_t*)decoded_BGR_mat.data, AV_PIX_FMT_BGR24, decoded_frameBGR->width, decoded_frameBGR->height, linesize_align);
    
  } 


  
  void copy_tmp_to_decoded()
  {
    if( false == isinit )
      {
	return;
      }
    int alignsize = 32;

    tmp_decoded_frame_buffer_mux.lock();
    
    
    if( NULL == decoded_frame )
      {
	fprintf(stdout, "REALLOC decoded (vid) frame\n");
	decoded_frame = av_frame_alloc();
	decoded_frame->format = tmp_decoded_frame_buffer.front()->format;
	decoded_frame->width = tmp_decoded_frame_buffer.front()->width;
	decoded_frame->height = tmp_decoded_frame_buffer.front()->height;
	decoded_frame->channels = tmp_decoded_frame_buffer.front()->channels;
	decoded_frame->channel_layout = tmp_decoded_frame_buffer.front()->channel_layout;
	decoded_frame->nb_samples = tmp_decoded_frame_buffer.front()->nb_samples;
	av_frame_get_buffer(decoded_frame, alignsize); 
      }
    
    av_frame_copy_props(decoded_frame, tmp_decoded_frame_buffer.front()); 
    av_frame_copy(decoded_frame, tmp_decoded_frame_buffer.front());


    av_frame_unref( tmp_decoded_frame_buffer.front() );
    av_frame_free( & tmp_decoded_frame_buffer.front() );
    tmp_decoded_frame_buffer.pop(); 
    
    tmp_decoded_frame_buffer_mux.unlock();
  }
  
  
  void copy_tmp_aud_to_decoded()
  {
    if( false == isinit )
      {
	return;
      }
    int alignsize = 32;
    
    
    if( NULL == decoded_aud_frame || decoded_aud_frame->nb_samples != tmp_decoded_aud_frame->nb_samples )
      {

	
	 
	
	decoded_aud_frame = av_frame_alloc();
	decoded_aud_frame->format = tmp_decoded_aud_frame->format;
	decoded_aud_frame->width = tmp_decoded_aud_frame->width;
	decoded_aud_frame->height = tmp_decoded_aud_frame->height;
	decoded_aud_frame->channels = tmp_decoded_aud_frame->channels;
	decoded_aud_frame->channel_layout = tmp_decoded_aud_frame->channel_layout;
	decoded_aud_frame->nb_samples = tmp_decoded_aud_frame->nb_samples;
	av_frame_get_buffer(decoded_aud_frame, alignsize); 
      }
    
    
    
    
    
    av_frame_copy_props(decoded_aud_frame, tmp_decoded_aud_frame);
    av_frame_copy(decoded_aud_frame, tmp_decoded_aud_frame);

    av_frame_unref( tmp_decoded_aud_frame );
  }
  
}; 




struct pts_sync_pkg
{
  int64_t ts;
  int64_t pts;
  int64_t pv;

  int64_t get_time()
  {
    return pts;
  }
  
  pts_sync_pkg( const int64_t& _ts, const int64_t& _pts, const int64_t& _pv )
    : ts(_ts), pts(_pts), pv(_pv)
  {
  }

  pts_sync_pkg()
  {
    ts=0;
    pts=0;
    pv=0;
  }
}; 



struct marker2d_pkg
{
  int64_t ts;
  std::vector<float> marker2d;

  marker2d_pkg( const int64_t& _ts, const std::vector<float>& _m2d )
    : ts(_ts), marker2d(_m2d)
  {
    if(2 != _m2d.size())
      {
	fprintf(stderr, "REV: wtf marker 2d vector size not 2?\n");
	exit(1);
      }
    
  }

  marker2d_pkg& operator=( const marker2d_pkg& other )
  {
    if( &other == this )
      {
	return *this;
      }

    ts = other.ts;
    marker2d = other.marker2d;
    
    return *this;
  }

  float getx()
  {
    if(marker2d.size() != 2)
      {
	fprintf(stderr, "exit\n");
	exit(1);
      }
    return marker2d[0];
  }

  float gety()
  {
    if(marker2d.size() != 2)
      {
	fprintf(stderr, "exit\n");
	exit(1);
      }
    return marker2d[1];
  }

  int64_t get_time()
  {
    return ts;
  }

  marker2d_pkg()
  {
    ts=0;
  }
}; 




struct gp_pkg
{
  int64_t ts;
  int64_t gidx;
  int64_t latency;
  std::vector<float> gp;

  
  gp_pkg& operator=( const gp_pkg& other )
  {
    if( &other == this )
      {
	return *this;
      }

    gp = other.gp;
    gidx = other.gidx;
    latency = other.latency;
    ts = other.ts;

    return *this;
  }

  float getx()
  {
    if( gp.size() < 2 )
      {
	fprintf(stderr, "REV: getx failed\n");
	exit(1);
      }
    return gp[0];
  }

  float gety()
  {
    if( gp.size() < 2 )
      {
	fprintf(stderr, "REV: getx failed\n");
	exit(1);
      }
    return gp[1];
  }

  int64_t get_time()
  {
    return ts;
  }

  gp_pkg( const int64_t& _ts, const int64_t& _gidx, const int64_t& _latency, const std::vector<float>& _gp )
    : ts(_ts), gidx(_gidx), latency(_latency), gp(_gp)
  {
  }

  gp_pkg()
  {
    ts=0;
    
  }
};



struct gp3_pkg
{
  long long ts;
  long gidx;
  std::vector<float> gp3;

  gp3_pkg( const long long& _ts, const long& _gidx, const std::vector<float>& _gp3 )
    : ts(_ts), gidx(_gidx), gp3(_gp3)
  {
  }
};


#define TS_TIMEBASE_HZ_SEC 9e4
struct data_pkg_buffer
{
  std::mutex mux;
  bool isinit;
  int64_t last_pv;
  int64_t last_pts;
  int64_t last_ts;
  int64_t zeroed_ts;
  
  rotating_timed_buffer<gp_pkg> gps;
  rotating_timed_buffer<marker2d_pkg> m2ds;
  
  
  data_pkg_buffer( const size_t& gpbufsize=200, const size_t& m2dsbufsize=20 )
  {
    gps.init( gpbufsize );
    m2ds.init( m2dsbufsize );
    isinit=false;
    last_pv=0;
    last_pts=0;
    last_ts=0;
    zeroed_ts=0;
  }

   

  
  int64_t pts_as_ts( const int64_t& pts )
  {
    int64_t ts = (pts * 1e6) / TS_TIMEBASE_HZ_SEC;
    return ts;
  }

  int64_t rebased_pts_as_ts( const int64_t& pts )
  {
    int64_t ts = pts_as_ts( pts );
    ts += zeroed_ts;
    return ts;
  }
  
  
  void sync_pkg( const pts_sync_pkg& ptssync )
  {
        
    last_pv = ptssync.pv;
    last_pts = ptssync.pts;
    last_ts = ptssync.ts;
    
    int64_t ptsus = pts_as_ts( last_pts ); 
    int64_t ts_minus_ptsus_diff = last_ts - ptsus; 
    
    if( true == isinit )
      {
#if DEBUG>5
	if( ts_minus_ptsus_diff != zeroed_ts )
	  {

	    fprintf(stderr, "REV: subsequent sync packages have different zeroed difference, old [%ld] versus new [%ld]\n", zeroed_ts, ts_minus_ptsus_diff );
	  }
#endif
      }
    
    zeroed_ts = ts_minus_ptsus_diff;
    
    isinit = true;
  }

  void add_marker2d( marker2d_pkg& m2d )
  {
    m2ds.add( m2d );
  }
  
  void add_gp( gp_pkg& gp )
  {
    gps.add( gp );
  }


  bool get_m2d_of_pts( const int64_t& pts, int64_t& maxoffset_us, float& ret_x, float& ret_y, const bool& flush_upto=true )
  {
    if( false == isinit )
      {
	return false;
      }

    int64_t ts = rebased_pts_as_ts( pts );
    marker2d_pkg m2d;
    int32_t idx = m2ds.get_straddled( ts, m2d, maxoffset_us );
        
    if( idx > 0 )
      {
	ret_x = m2d.getx();
	ret_y = m2d.gety();
	
	return true;
      }
    else
      {
	return false;
      }
    if( flush_upto && idx >= 0 )
      {
	m2ds.flush_less_than_idx( idx );
      }
  }
  
  bool get_gp_of_pts( const int64_t& pts, int64_t& maxoffset_us, float& ret_x, float& ret_y, const bool& flush_upto=true )
  {
    if( false == isinit )
      {
	
	return false;
      }
    
    int64_t ts = rebased_pts_as_ts( pts );
    
    gp_pkg gp;
    int32_t idx = gps.get_straddled( ts, gp, maxoffset_us );
    
    if( idx > 0 )
      {
	ret_x = gp.getx();
	ret_y = gp.gety();
	if( flush_upto )
	  {
	    gps.flush_less_than_idx( idx );
	  }
	return true;
      }
    else
      {
	return false;
      }
    
  }
    
}; 





struct tobii_data_dumper
{

  rteye2::Timer t;
  
  std::string dumpfilebase;
  std::string scenedumpfilename;
  std::string sceneidxfilename;

  std::string eyedumpfilename;
  std::string eyeidxfilename;

  std::string datadumpfilename;
  std::string dataidxfilename;

  std::ofstream scenedump;
  std::ofstream eyedump;
  std::ofstream datadump;
  std::ofstream sceneidx;
  std::ofstream eyeidx;
  std::ofstream dataidx;

  std::ifstream rscenedump;
  std::ifstream reyedump;
  std::ifstream rdatadump;
  std::ifstream rsceneidx;
  std::ifstream rdataidx;
  std::ifstream reyeidx;

  std::list< std::vector<char> > scenebuf;
  std::list< std::vector<char> > eyebuf;
  std::list< std::vector<char> > databuf;
  std::list< double > scenetimebuf;
  std::list< double > eyetimebuf;
  std::list< double > datatimebuf;

  
  bool_condvar scenebufcv;
  bool_condvar eyebufcv;
  bool_condvar databufcv;
  

  std::thread scenethread;
  std::thread eyethread;
  std::thread datathread;

  bool looper;
  
  void init( const std::string& _dumpfile="")
  {
    dumpfilebase = _dumpfile; 
    scenedumpfilename=dumpfilebase+"_dumpscene.dump";
    eyedumpfilename=dumpfilebase+"_dumpeye.dump";
    datadumpfilename=dumpfilebase+"_dumpdata.dump";
    sceneidxfilename=dumpfilebase+"_dumpscene.idx";
    eyeidxfilename=dumpfilebase+"_dumpeye.idx";
    dataidxfilename=dumpfilebase+"_dumpdata.idx";
    looper = false;
  }
  
  bool empty() const
  {
    return dumpfilebase.empty();
  }

  
  
  void adddataBLOCKING( const char* ptr, const size_t size_bytes )
  {
    dataidx << t.elapsed() << " " << size_bytes << std::endl;
    datadump.write( ptr, size_bytes );
  }

  void adddata( const char* ptr, const size_t size_bytes )
  {
    
    double time = t.elapsed();
    std::vector<char> v(size_bytes);
    std::memcpy( v.data(), ptr, size_bytes );

    
    {
      
      std::unique_lock<std::mutex> lk( databufcv.mux );
      databuf.push_back( v );
      datatimebuf.push_back( time );
      databufcv.val = true;
    }
    databufcv.cv.notify_all();
    
    
  }

  void addsceneBLOCKING( const char* ptr, const size_t size_bytes )
  {
    sceneidx << t.elapsed() << " " << size_bytes << std::endl;
    scenedump.write( ptr, size_bytes );
  }

  void addscene( const char* ptr, const size_t size_bytes )
  {
    
    double time = t.elapsed();
    std::vector<char> v(size_bytes);
    std::memcpy( v.data(), ptr, size_bytes );
    
    
    {
      
      std::unique_lock<std::mutex> lk( scenebufcv.mux );
      scenebuf.push_back( v );
      scenetimebuf.push_back( time );
      scenebufcv.val = true;
    }
    scenebufcv.cv.notify_all();
    
    
  }

  void addeyeBLOCKING( const char* ptr, const size_t size_bytes )
  {
    eyeidx << t.elapsed() << " " << size_bytes << std::endl;
    eyedump.write( ptr, size_bytes );
  }

  void addeye( const char* ptr, const size_t size_bytes )
  {
    
    double time = t.elapsed();
    std::vector<char> v(size_bytes);
    std::memcpy( v.data(), ptr, size_bytes );
    
    
    {
      
      std::unique_lock<std::mutex> lk( eyebufcv.mux );
      eyebuf.push_back( v );
      eyetimebuf.push_back( time );
      eyebufcv.val = true;
    }
    eyebufcv.cv.notify_all();
    
    
  }
  
  void write_disk_loop( std::list< std::vector<char> >& buf, std::list<double>& timebuf, bool_condvar& cv, bool& loop, std::ofstream& bufos, std::ofstream& idxos )
  {
    fprintf(stdout, "Starting write_disk_loop\n");
    while( loop )
      {
	
	{
	  std::unique_lock<std::mutex> lk( cv.mux );
	  while( false == cv.val && true == loop )
	    {
	      cv.cv.wait_for( lk, std::chrono::milliseconds( 10 ) );
	    }
	
	  if( buf.size() > 0 && timebuf.size() > 0 )
	    {
	      idxos << timebuf.front() << " " << buf.front().size() << std::endl;
	      bufos.write( buf.front().data(), buf.front().size() );
	    
	      timebuf.pop_front();
	      buf.pop_front();
	    }
	  else
	    {
	      cv.val = false;
	    }
	}
	cv.cv.notify_all();
	
      }
    
    std::unique_lock<std::mutex> lk( cv.mux );
    while( buf.size() > 0 && timebuf.size() > 0 )
      {
	fprintf(stdout, "Finishing writing out raw data (write_disk_loop)\n");
	idxos << timebuf.front() << " " << buf.front().size() << std::endl;
	bufos.write( buf.front().data(), buf.front().size() );
	      
	
	timebuf.pop_front();
	buf.pop_front();
      }
    fprintf(stdout, "Finished writing out raw data (write_disk_loop)\n");
  }
  
  void openfiles()
  {
    scenedump.open( scenedumpfilename, std::ofstream::out | std::ofstream::binary );
    eyedump.open( eyedumpfilename, std::ofstream::out | std::ofstream::binary );
    datadump.open( datadumpfilename, std::ofstream::out | std::ofstream::binary );
    
    sceneidx.open( sceneidxfilename, std::ofstream::out );
    eyeidx.open( eyeidxfilename, std::ofstream::out );
    dataidx.open( dataidxfilename, std::ofstream::out );

    if( !scenedump.is_open() || !datadump.is_open() || !sceneidx.is_open() || !dataidx.is_open()
	|| !eyeidx.is_open() || !eyeidx.is_open())
      {
	fprintf(stderr, "REV: error opening data dumper files (possibly wrong filenames?) E.g. [%s]. Exiting (note could be eye or data file too)\n", scenedumpfilename.c_str() );
	exit(1);
      }

    fprintf(stdout, "Starting dumper threads!\n");
    looper = true;
    
    scenethread = std::thread(  &tobii_data_dumper::write_disk_loop, this, std::ref(scenebuf), std::ref(scenetimebuf), std::ref(scenebufcv), std::ref(looper), std::ref( scenedump ), std::ref( sceneidx ) );

    eyethread = std::thread(  &tobii_data_dumper::write_disk_loop, this, std::ref(eyebuf), std::ref(eyetimebuf), std::ref(eyebufcv), std::ref(looper), std::ref( eyedump ), std::ref( eyeidx ) );

    datathread = std::thread(  &tobii_data_dumper::write_disk_loop, this, std::ref(databuf), std::ref(datatimebuf), std::ref(databufcv), std::ref(looper), std::ref(datadump), std::ref(dataidx) );
  }

  void openread()
  {
    rscenedump.open( scenedumpfilename, std::ifstream::in | std::ifstream::binary );
    reyedump.open( eyedumpfilename, std::ifstream::in | std::ifstream::binary );
    rdatadump.open( datadumpfilename, std::ofstream::in | std::ofstream::binary );
    
    rsceneidx.open( sceneidxfilename, std::ofstream::in );
    reyeidx.open( eyeidxfilename, std::ofstream::in );
    rdataidx.open( dataidxfilename, std::ofstream::in );

    if( !rscenedump.is_open() || !rdatadump.is_open() || !rsceneidx.is_open() || !rdataidx.is_open()
	|| !reyeidx.is_open() || !reyeidx.is_open()	)
      {
	fprintf(stderr, "REV: error opening data dumper READ files (possibly wrong filenames?) E.g. [%s]. Exiting (could be eye or data)\n", scenedumpfilename.c_str() );
	exit(1);
      }
  }

  ~tobii_data_dumper()
  {
    fprintf(stdout, "Waiting for tobii data dumper threads\n");
    looper = false;
    fprintf(stdout, "Joining scene thread...\n");
    if( scenethread.joinable() )
      {
	scenethread.join();
      }
    fprintf(stdout, "Joining eye thread...\n");
    if( eyethread.joinable() )
      {
	eyethread.join();
      }
    fprintf(stdout, "Joining data thread...\n");
    if( datathread.joinable() )
      {
	datathread.join();
      }
    fprintf(stdout, "Done joining all dumper threads\n");
  }
};

struct tobii_dumped_server
{
  std::string KA_DATA_MSG;
  std::string KA_VIDEO_MSG;
  std::string KA_EYE_MSG;
  std::string STOP_DATA_MSG;
  std::string STOP_VIDEO_MSG;
  std::string STOP_EYE_MSG;
  
  tobii_data_dumper dumper;

  std::thread scenesender;
  std::thread datasender;
  std::thread eyesender;

  std::thread keepalives;
  
  int serversocket;

  struct sockaddr_in si_vid;
  struct sockaddr_in si_dat;
  struct sockaddr_in si_eye;
  bool senddata;
  bool sendvid;
  bool sendeye;
  
  bool loop;

  std::string ip4addr;
  int port;
  
  
  
  
  

  

  
  
  

  
  
  

  tobii_dumped_server()
  {
    senddata=false;
    sendvid = false;
    sendeye = false;
    serversocket= -1;

    
    KA_DATA_MSG = "{\"type\": \"live.data.unicast\", \"key\": \"some_GUID\", \"op\": \"start\"}";
    KA_VIDEO_MSG = "{\"type\": \"live.video.unicast\", \"key\": \"some_other_GUID\", \"op\": \"start\"}";
    KA_EYE_MSG = "{\"type\": \"live.eyes.unicast\", \"key\": \"some_third_GUID\", \"op\": \"start\"}";
    
    STOP_DATA_MSG = "{\"type\": \"live.data.unicast\", \"key\": \"some_GUID\", \"op\": \"stop\"}";
    STOP_VIDEO_MSG = "{\"type\": \"live.video.unicast\", \"key\": \"some_other_GUID\", \"op\": \"stop\"}";
    STOP_EYE_MSG = "{\"type\": \"live.eyes.unicast\", \"key\": \"some_third_GUID\", \"op\": \"start\"}";
    
  }

  ~tobii_dumped_server()
  {
    loop = false;
    if(keepalives.joinable())
      {
	keepalives.join();
      }
    if(scenesender.joinable())
      {
	scenesender.join();
      }
    if( datasender.joinable())
      {
	datasender.join();
      }
    if( eyesender.joinable())
      {
	eyesender.join();
      }
  }
  
  void start( const std::string& fname )
  {
    loop=true;
    dumper.init( fname );
    dumper.t.reset();
    keepalives = std::thread( &tobii_dumped_server::keepalives_funct, this );
    scenesender = std::thread( &tobii_dumped_server::scenesender_funct, this );
    datasender = std::thread( &tobii_dumped_server::datasender_funct, this );
    eyesender = std::thread( &tobii_dumped_server::eyesender_funct, this );
  }

  

  
  
  
  
  void keepalives_funct( )
  {
    
    
    double maxwait=10.0; 
    rteye2::Timer t;
    size_t size=100;
    std::vector<char> buf(size);
    struct sockaddr_in si_other;
    
    
    if( serversocket < 0 )
      {
	serversocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (serversocket < 0)
	  {
	    fprintf(stderr, "ERROR: CONNECT_TO_EYETRACKER_SOCKET: Could not create socket\n");
	    exit(1);
	  }
      
	struct sockaddr_in serv_addr;
      
	bzero((char *) &serv_addr, sizeof(serv_addr));
      
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port); 
      
	int ret = bind(serversocket, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	if( ret < 0 )
	  {
	    fprintf(stderr, "REV: bind failed [%s]\n", strerror(errno));
	    exit(1);
	  }
      }
    
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    if (setsockopt(serversocket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0)
      {
	fprintf(stderr, "Could not set socket option timeout\n");
	exit(1);
      }
    int got=0;
    uint32_t slen;
    
    while( loop )
      {
	
	got = recvfrom(serversocket, buf.data(), buf.size(), 0, (struct sockaddr *) &si_other, &slen );
	if( got <= 0 )
	  {
	    if( t.elapsed() > maxwait )
	      {
		fprintf( stderr, "SERVER: keepalives timed out\n");
		loop=false;
		break;
	      }
	    
	  }
	else
	  {
	    
	    std::string r( buf.begin(), buf.begin()+got );
	    if( 0 ==  r.compare( KA_DATA_MSG ) )
	      {
		si_dat = si_other;
		senddata = true;
	      }
	    else if( 0 == r.compare( KA_VIDEO_MSG ) )
	      {
		si_vid = si_other;
		sendvid = true;
	      }
	    else
	      {
		fprintf(stderr, "REV: server received unknown keepalive message [%s]\n", r.c_str() );
		exit(1);
	      }
	      
	    fprintf(stdout, "SERVER: scenekeepalive received [%s] after [%lf] sec\n", r.c_str(), t.elapsed() );

	    t.reset();
	  }
      }
  }

  void scenesender_funct()
  {
    double time;
    size_t bytes;
    std::vector<char> tmpbuf;
    while( loop )
      {
	dumper.rsceneidx >> time >> bytes;
	if( dumper.rsceneidx.fail() )
	  {
	    fprintf(stderr, "Scene sender index ran out of lines\n");
	    loop = false;
	    break;
	  }
	else
	  {
	    tmpbuf.resize( bytes );
	    dumper.rscenedump.read( tmpbuf.data(), bytes );
	    if( dumper.rscenedump.fail() )
	      {
		fprintf(stderr, "Scene sender dumper ran out of bytes?\n");
		loop = false;
		break;
	      }
	    int64_t tosleep_nsec = time*1e9 - dumper.t.elapsed()*1e9;
	    if( tosleep_nsec > 0 )
	      {
		std::this_thread::sleep_for( std::chrono::nanoseconds(tosleep_nsec) );
	      }
	    
	    
	    uint32_t slen;
	    int ret = sendto( serversocket, tmpbuf.data(), tmpbuf.size(), 0, (struct sockaddr*) &si_vid, slen );
	    if( ret < 0 )
	      {
		fprintf(stderr, "SERVER REV: error in sendto vid\n");
		exit(1);
	      }
	  }
      }
  }

   void eyesender_funct()
  {
    double time;
    size_t bytes;
    std::vector<char> tmpbuf;
    while( loop )
      {
	dumper.reyeidx >> time >> bytes;
	if( dumper.reyeidx.fail() )
	  {
	    fprintf(stderr, "Eye sender index ran out of lines\n");
	    loop = false;
	    break;
	  }
	else
	  {
	    tmpbuf.resize( bytes );
	    dumper.reyedump.read( tmpbuf.data(), bytes );
	    if( dumper.reyedump.fail() )
	      {
		fprintf(stderr, "EYE sender dumper ran out of bytes?\n");
		loop = false;
		break;
	      }
	    int64_t tosleep_nsec = time*1e9 - dumper.t.elapsed()*1e9;
	    if( tosleep_nsec > 0 )
	      {
		std::this_thread::sleep_for( std::chrono::nanoseconds(tosleep_nsec) );
	      }
	    
	    
	    uint32_t slen;
	    int ret = sendto( serversocket, tmpbuf.data(), tmpbuf.size(), 0, (struct sockaddr*) &si_vid, slen );
	    if( ret < 0 )
	      {
		fprintf(stderr, "SERVER REV: error in sendto eye\n");
		exit(1);
	      }
	  }
      }
  }

  void datasender_funct()
  {
    double time;
    size_t bytes;
    std::vector<char> tmpbuf;
    while( loop )
      {
	dumper.rdataidx >> time >> bytes;
	if( dumper.rdataidx.fail() )
	  {
	    fprintf(stderr, "data sender index ran out of lines\n");
	    loop = false;
	    break;
	  }
	else
	  {
	    tmpbuf.resize( bytes );
	    dumper.rdatadump.read( tmpbuf.data(), bytes );
	    if( dumper.rdatadump.fail() )
	      {
		fprintf(stderr, "data sender dumper ran out of bytes?\n");
		loop = false;
		break;
	      }
	    int64_t tosleep_nsec = time*1e9 - dumper.t.elapsed()*1e9;
	    if( tosleep_nsec > 0 )
	      {
		std::this_thread::sleep_for( std::chrono::nanoseconds(tosleep_nsec) );
	      }
	    
	    
	    uint32_t slen;
	    int ret = sendto( serversocket, tmpbuf.data(), tmpbuf.size(), 0, (struct sockaddr*) &si_dat, slen );
	    if( ret < 0 )
	      {
		fprintf(stderr, "SERVER REV: error in sendto dat\n");
		exit(1);
	      }
	  }
      }
  }
  
};





#include <utils.hpp>


struct eyepos_reader
{
  

  size_t idx;
  int64_t starttime;
  int64_t time;
  float x;
  float y;
  bool done;

  std::vector<int64_t> times;
  std::vector<float> xs;
  std::vector<float> ys;
  
  eyepos_reader() {}

  eyepos_reader( const std::string& fname )
  {
    
    
    fprintf(stdout, "Opening eyestream from [%s]\n", fname.c_str() );
    std::ifstream eyestream;
    eyestream.open( fname );
    if( !eyestream.is_open() )
      {
	fprintf(stderr, "REV: no eyepos.txt file found in same directory as the selected rawScene, or error reading eyestream filename [%s]. Will not perform eyetracking\n", fname.c_str());
	return;
	
      }

     
    
    
    
    std::string c1, c2, c3;
    eyestream >> c1 >> c2 >> c3;
    if( !is_same_string( c1, "TIME_MS" ) || !is_same_string( c2, "EYEX" ) || !is_same_string( c3, "EYEY" ) )
      {
	fprintf(stderr, "EYE FILE HEADER INCORRECT\n");
	exit(1);
      }
    
    while( eyestream >> time >> x >> y )
      {
	times.push_back( time );
	xs.push_back(x);
	ys.push_back(y);
      }
    starttime=0;
    idx = 0;
    done=false;
    
    set_next();

    
    if( done || x > -0.999 || y > -0.999 )
      {
	fprintf(stderr, "REV: reading from eyepos file, DONE BEFORE STARTED, or first line should have time of first vid frame (msec) followed by two very negative numbers (-1e12) to represent x and y. We got [%ld] [%lf] [%lf]\n", starttime, x, y);
	exit(1);
      }
    starttime = time;
    set_next();
    fprintf(stdout, "Finished loading eyestream, starttime [%ld], (curr idx=%ld, should be 2!), first time [%ld], first x/y: [%f][%f], [%ld] eye measurements\n", starttime, idx, times[0], xs[0], ys[0], times.size()-1 );
  }


  void set_next()
  {
    done = ( idx >= times.size() ? true : false );
    if(false == done)
      {
	time = times[idx] - starttime; 
	x = xs[idx];
	y = ys[idx];
	++idx;
      }
  }
  
  
  
  
  
  

  
  
  
  bool get_eyepos( const int64_t& currtime, eyepos& ep ) 
  {
    
    int64_t legalstart = currtime - 20;
    int64_t legalend =   currtime + 19;
    
    
    
    
    while( time < legalstart && false == done)
      {
	
	set_next();
      }
    if( false == done &&
	time <= legalend &&
	time >= legalstart )
      {
	ep = eyepos( x, y );
	return true;
      }
    else
      {
	return false;
      }
  }
};
