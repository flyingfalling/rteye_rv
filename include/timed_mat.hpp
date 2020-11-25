
#pragma once

struct mat_timing
{
  int64_t idx; 
  int64_t pts;
  
  
  
  
  
  

  inline mat_timing()
  {
    idx=0;
    pts=0;
  }
  
  inline void copy_timing( const AVFrame* frame )
  {
    ++idx; 
    pts = frame->pts;
    
    
    
    
    
    
    
  }
};

struct timed_mat
{
  cv::Mat mat;
  mat_timing mt;

  inline timed_mat()
  {
    
  }
  
  
  
  
  
  inline timed_mat& operator=( const timed_mat& other )
  {
    
    if( &other == this )
      {
	
	return *this;
      }

    
    if( false == other.mat.empty() )
      {
	other.mat.copyTo( mat );
      }
        
    this->mt = other.mt;

    return *this;
  }

  inline int64_t get_time()
  {
    return mt.pts;
  }
  
  inline void set_timing( const mat_timing& _mt )
  {
    mt = _mt;
  }
}; 




struct timed_audio_frame
{
  int64_t idx;
  int64_t pts;
  std::vector<uint8_t> data;
  
  inline timed_audio_frame( const size_t& bytesize=1152 )
  {
    idx=0;
    pts=0;
    data.resize( bytesize );
    pts=-1;
  }
  
  inline timed_audio_frame& operator=( const timed_audio_frame& other )
  {
    if( &other == this )
      {
	return *this;
      }
    
    idx = other.idx;
    pts = other.pts;
    data.resize( other.data.size() );
    std::memcpy( data.data(), other.data.data(), other.data.size() );
    
    return *this;
  }
  
  inline int64_t get_time()
  {
    return pts;
  }
  
  
  
  inline void add_data( const AVFrame* frame, AVCodecContext* codeccontext )
  {
    if( NULL == frame )
      {
	return;
      }
    if( 1 != frame->channels )
      {
	fprintf(stderr, "REV: more than 1 audio channel, exiting (note frame->channels)\n");
	
	return;
      }

    if( -1 == frame->format )
      {
	fprintf(stderr, "REV: frame format unknown\n");
	
	return;
      }

    int data_size = av_samples_get_buffer_size(NULL, codeccontext->channels,
					       frame->nb_samples,
					       codeccontext->sample_fmt, 1);
    
    
    data.resize( frame->nb_samples * av_get_bytes_per_sample( (AVSampleFormat)frame->format ) );

    if( data_size != data.size() )
      {
	fprintf(stderr, "REV: something is wrong in audio size\n");
	
	return;
      }
    
    std::memcpy( data.data(), frame->data[0], data.size() ); 
    pts = frame->pts;
    
    
  }
  
  inline void printdata()
  {
    for(size_t x=0; x<data.size(); x+=2) 
      {
	fprintf(stdout, "[%6d]", *(int16_t*)(data.data()+(x)) );
      }
    fprintf(stdout, "\n");
  }
};
