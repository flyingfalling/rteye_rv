#pragma once


#include <mutex>
#include <map>

#include <cstdlib>
#include <cstdio>
 #include <sys/types.h>
 #include <sys/stat.h>
#include <pwd.h>
#include <fstream>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif  
#include <memory>





bool copyFile(const std::string& SRC, const std::string& DEST);








bool samestring( const std::string& a, const std::string& b );

std::string replace_illegals( const std::string& s );

int do_mkdir(const char *path, mode_t mode);

int mkpath(const std::string& path, mode_t mode);

class Env
{
public:
  static std::string getUserName();
};


#define WRITE_U32(buf, x) *(buf)     = (unsigned char)((x)&0xff);	\
  *((buf)+1) = (unsigned char)(((x)>>8)&0xff);				\
  *((buf)+2) = (unsigned char)(((x)>>16)&0xff);				\
  *((buf)+3) = (unsigned char)(((x)>>24)&0xff);

#define WRITE_U16(buf, x) *(buf)     = (unsigned char)((x)&0xff);	\
  *((buf)+1) = (unsigned char)(((x)>>8)&0xff);



void add_timing( const int64_t& msec, cv::Mat& m);
void add_name( const std::string& name, cv::Mat& m);


struct eyepos
{
  float x;
  float y;

  eyepos()
    : x(-1e3), y(-1e3)
  {
  }
  
  eyepos( const float& x1, const float& y1 )
  {
    x = x1;
    y = y1;
  }
  
  void set_xy( const float& x1, const float& y1 )
  {
    x=x1;
    y=y1;
  }

  void get_xy( float& x1, float& y1 )
  {
    x1 = x;
    y1 = y;
  }

  void get_xy_pix( int& eyex_pix, int& eyey_pix, const cv::Mat& mymat )
  {
    eyex_pix = x * mymat.size().width;
    eyey_pix = y * mymat.size().height; 
  }

  void get_xy_pix_centered( int& eyex_pix, int& eyey_pix, const cv::Mat& mymat )
  {
    size_t w2 = mymat.size().width / 2;
    size_t h2 = mymat.size().height / 2;
    eyex_pix = w2 + (w2 * x);
    eyey_pix = h2 + (h2 * y);
  }
};

struct saved_roc
{
  std::string fname;
  std::ofstream of;

  saved_roc()
  {
  }
  
  
  saved_roc(saved_roc&& arg)
    : of(std::move(arg.of))
  {
  }

  saved_roc& operator=(saved_roc&& arg)
  {
      of = std::move(arg.of);
      return *this;
    }
  
   

  void init( const std::string& myfname )
  {
    fname = myfname;
    of.open( fname, std::ofstream::out );
    fprintf( stdout, "Opening roc file [%s]\n", fname.c_str() );
    fflush(stdout);
    if(!of)
      {
	fprintf(stderr, "REV: error opening fstream\n");
	exit(1);
      }
    of << "TIME_MS" << " " << "ROC" << " " << "FAIL_RATIO" << std::endl;
  }

  void add_roc(const int64_t& time_msec, const float& rocval, const float& failratio)
  {
    if( of << time_msec << " " << rocval << " " << failratio << std::endl )
      {
	
      }
    else
      {
	fprintf(stderr,"ROC out fail\n");
	exit(1);
      }
  }
  ~saved_roc()
  {
    of.close();
  }
};


struct saved_eyepos
{
  std::string fname;
  std::ofstream of;

  saved_eyepos()
  {
  }

   

  void init( const std::string& myfname )
  {
    fname = myfname;
    of.open( fname, std::ofstream::out );
    fprintf( stdout, "Opening eyepos file [%s]\n", fname.c_str() );
    fflush(stdout);
    if(!of)
      {
	fprintf(stderr, "REV: error opening fstream\n");
	exit(1);
      }
    of << "TIME_MS" << " " << "EYEX" << " " << "EYEY" << std::endl;
  }

  void add_eyepos( const int64_t& time_msec, const float& eyex, const float& eyey )
  {
    if( of << time_msec << " " << eyex << " " << eyey << std::endl )
      {
	
      }
    else
      {
	
	fprintf(stdout, "Error outputting to eyepos file (printing %ld, %f %f)\n", time_msec, eyex, eyey);
	if( of.fail() )
	  {
	    fprintf(stderr,"failed\n");
	  }
	else if( of.bad() )
	  {
	    fprintf(stderr, "Stream bad\n");
	  }
	else if( of.eof() )
	  {
	    fprintf(stderr, "Its eof\n");
	  }
	else
	  {
	    fprintf(stderr, "None of the above\n");
	  }
	exit(1);
      }
  }
  
  ~saved_eyepos()
  {
    of.close();
  }
};

struct saved_wav 
{
  int quiet;
  int bits;
  int endian;
  int raw;
  int sign;
  int64_t written;
  FILE* out;
  std::string fname;
  unsigned char headbuf[44];   
  
  
  

   

  bool writing()
  {
    return (NULL != out);
  }
  
  saved_wav()
  {
    quiet = 0;
    bits = 16;
    endian = 0;
    raw = 0;
    sign= 1;
    written=0;
    out=NULL;
    fname ="tobiiaudio.wav";
  }

   

  void init( const std::string& myfname, const int& samplerate=24000, const int& channels=1  )
  {
    fname = myfname;
    out = fopen( fname.c_str(), "wb" );
    if( NULL == out )
      {
	fprintf(stderr, "REV: error opening wav output file [%s]\n", fname.c_str());
	exit(1);
      }
    write_prelim_header( channels, samplerate );
    written=0;
  }
  
  int write_prelim_header (int channels=1, int samplerate=24000)
  {
    
    int knownlength = 0;

    unsigned int size = 0x7fffffff;
    
    
    int bytespersec = channels * samplerate * bits / 8;
    int align = channels * bits / 8;
    int samplesize = bits;
    
    if (knownlength)
      { size = (unsigned int) knownlength; }
    
    memcpy (headbuf, "RIFF", 4);
    WRITE_U32 (headbuf + 4, size - 8);
    memcpy (headbuf + 8, "WAVE", 4);
    memcpy (headbuf + 12, "fmt ", 4);
    WRITE_U32 (headbuf + 16, 16);
    WRITE_U16 (headbuf + 20, 1);   
    WRITE_U16 (headbuf + 22, channels);
    WRITE_U32 (headbuf + 24, samplerate);
    WRITE_U32 (headbuf + 28, bytespersec);
    WRITE_U16 (headbuf + 32, align);
    WRITE_U16 (headbuf + 34, samplesize);
    memcpy (headbuf + 36, "data", 4);
    WRITE_U32 (headbuf + 40, size - 44);

    if( out )
      {
	if (fwrite (headbuf, 1, 44, out) != 44)
	  {
	    printf ("ERROR: Failed to write wav header: %s\n", strerror (errno));
	    return 1;
	  }
      }

    return 0;
  }

  void write_data( const std::vector<uint8_t>& dat )
  {
    
    int64_t nwritten = fwrite( dat.data(), sizeof(uint8_t), dat.size(), out );
    if( nwritten != dat.size() )
      {
	fprintf(stderr, "REV: error writing data to wav file?\n");
	exit(1);
      }

    written += nwritten; 
  }
  
  
  int rewrite_header()
  {
    unsigned int length = written;

    length += 44;

    WRITE_U32 (headbuf + 4, length - 8);
    WRITE_U32 (headbuf + 40, length - 44);
    if (fseek (out, 0, SEEK_SET) != 0)
      {
	printf ("ERROR: Failed to seek on seekable file: %s\n",
		strerror (errno));
	return 1;
      }

    if (fwrite (headbuf, 1, 44, out) != 44)
      {
	printf ("ERROR: Failed to write wav header: %s\n", strerror (errno));
	return 1;
      }
    return 0;
  }

  ~saved_wav()
  {
    
    if( NULL != out )
      {
	fprintf(stdout, "Rewriting AUDIO header and exiting...\n");
	rewrite_header();
	
	fclose(out);
	out = NULL;
      }
  }
  
};


struct saved_vid
{
  std::string name;
  std::string fname;
  cv::Size size;
  std::shared_ptr<cv::VideoWriter> vw;
  double fps;
  
  saved_vid( const std::string& vidname, const std::string& savename, const cv::Size& mysize, const int& myfps, const bool lossy=true )
  {
    name=vidname;
    fname=savename;
    size=mysize;
    fps = myfps;
    int fourcc;
    int api = cv::CAP_FFMPEG; 
     
    
    bool iscolor=true;
    if( true == lossy )
      {
	
	fourcc = cv::VideoWriter::fourcc('M', 'P', 'E', 'G');
	
	
	
		
	
	
	
	
	
	

      }
    else
      {
	
	fourcc = cv::VideoWriter::fourcc('F', 'F', 'V', '1');
	
      }
    
    vw = std::make_shared<cv::VideoWriter>( fname, fourcc, fps, size, iscolor );
    if( false == vw->isOpened() )
      {
	fprintf(stderr, "REV: error opening video [%s] (w=%d h=%d, fps=%d)\n", savename.c_str(), mysize.width, mysize.height, myfps );
	exit(1);
      }
  }

  saved_vid()
  {
    fname="vid.mpeg";
  }

  void add_frame( const cv::Mat& m )
  {
    if( m.size() != size )
      {
	fprintf(stderr, "REV: attempting to add incorrect size video to saved_vid (vid size is %d %d but adding %d %d) Vid name [%s] fname [%s]\n", size.width, size.height, m.size().width, m.size().height, name.c_str(), fname.c_str() );
	exit(1);
      }
    (*vw) << m;
  }

   
  
};

struct vid_saver
{
  std::string path="./";
  
  std::map<std::string, saved_vid> vids;
  saved_wav audio;
  saved_eyepos eyepos;
  std::map<std::string,saved_roc> roc;
  std::mutex mux;
  
  bool started; 
  
  vid_saver( const std::string& savepath )
  {
    started=false;
    std::string saveviddir2;
    if( savepath.compare( "" ) != 0 )
      {
	if( savepath[0] != '/' )
	  {
	    
	    std::string username = Env::getUserName();
	    
	    saveviddir2 = "/home/" + username + "/" + savepath;
	    fprintf(stdout, "Saving videos in directory [%s]\n", saveviddir2.c_str() );
	  }
	else
	  {
	    saveviddir2 = savepath;
	    fprintf(stdout, "Saving videos in directory [%s]\n", saveviddir2.c_str() );
	  }
      }

    
    int err = mkpath( saveviddir2, 0777 );
    if(err != 0 )
      {
	fprintf(stderr, "REV: could not make path [%s]\n", saveviddir2.c_str());
	exit(1);
      }

    path = saveviddir2;
  }

  vid_saver()
  {
    started=false;
    path="./";
  }
  
  void add_video( const std::string& name, const std::string& fname, const cv::Size& size,  const int& fps, const bool lossy=true)
  {
    
    size_t n = vids.count( name );
    if( n == 0 )
      {
	std::string fullname =  path+"/"+fname;
	vids[ name ] = saved_vid( name, fullname, size, fps, lossy );
	fprintf(stdout, "Opening video [%s] as [%s]\n", name.c_str(), fullname.c_str());
      }
    else
      {
	fprintf(stderr, "REV: adding multiple of same name.\n");
	exit(1);
      }
  }

  void open_audio( const std::string& fname )
  {
    std::string fullname = path + "/" + fname;
    fprintf(stdout, "Opening audio as [%s]\n", fullname.c_str());
    int samplerate=24000;
    audio.init(fullname, samplerate); 
    if( !audio.writing() )
      {
	fprintf(stdout, "REV: error initializing wav writer\n");
	exit(1);
      }
  }
  
  void add_to_audio( const std::vector<uint8_t>& bytes, const std::string& fname )
  {
    if( !audio.writing() )
      {
	open_audio( fname );
      }
    
    if( audio.writing() )
      {
	audio.write_data( bytes );
      }
  }

  void open_eyepos( const std::string& fname )
  {
    std::string fullname = path + "/" + fname;
    
    
    eyepos.init(fullname); 
    
  }

  

  void add_to_eyepos( const int64_t& timemsec, const float& x, const float& y, const std::string& fname )
  {
    
    if( !eyepos.of.is_open() )
      {
	
	open_eyepos( fname );
      }

    
    eyepos.add_eyepos( timemsec, x, y );
  }
  
  void open_roc( const std::string& fname )
  {
    std::string fullname = path + "/" + fname;
    
    
    roc[fname].init(fullname); 
    
  }
  
  void add_to_roc( const int64_t& timemsec, const float& rocval, const float& failratio, const std::string& fname )
  {
    if( roc.find( fname ) == roc.end() )
      {
	
	roc[fname] = saved_roc(); 
      }
    if( !roc[fname].of.is_open() )
      {
	
	open_roc( fname );
      }

    
    roc[fname].add_roc( timemsec, rocval, failratio );
  }
  
  void add_to_video( const std::string& name, const cv::Mat& m, const std::string& fname, const int& fps, const bool lossy=true )
  {
    mux.lock();
    auto myvid = vids.find(name);
    if( myvid == vids.end() )
      {
	add_video(name, fname, m.size(), fps, lossy);
	myvid = vids.find(name);
      }
    mux.unlock();
    
    
    myvid->second.add_frame(m);
  }
  
};
