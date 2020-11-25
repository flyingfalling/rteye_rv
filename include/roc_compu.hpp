



#pragma once

#include <vector>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <cstring>


#include <random>
#include <opencv2/opencv.hpp>
#include <utils.hpp>

#include <Timer.hpp>



float dist( const float& prevx, const float& prevy, const float& newx, const float& newy );

template <typename T>
void print_vec( const std::vector<T>& v1 );

template <typename T>
void vec_pointwise_sum( std::vector<T>& v1, const std::vector<T>& v2 );

template <typename T>
void vec_pointwise_diff( std::vector<T>& v1, const std::vector<T>& v2 );

template <typename T>
void vec_const_div( std::vector<T>& v1, const T& v2 );

template <typename T>
void vec_threshold( const std::vector<T>& input, std::vector<T>& output, const T& thresh);

template <typename T>
T vec_sum( const std::vector<T>& input );

float AUC_from_plot( const std::vector<float>& truepos, const std::vector<float>& falsepos );

template <typename T>
struct rotating_buffer
{
  size_t max_buf_size;
  size_t curr_size;
  size_t head; 
  std::vector<T> buf;

  rotating_buffer()
  {
    curr_size=0;
    max_buf_size=0;
    head=0;
    buf.resize( max_buf_size );
  }
  
  rotating_buffer( const size_t& max_size )
    : max_buf_size( max_size ), curr_size(0), head(0)
  {
    buf.resize( max_buf_size );
  }

  void init( const size_t& max_size )
  {
    max_buf_size = max_size;
    buf.resize( max_buf_size );
    head=0;
    curr_size=0;
  }
  
  size_t get_n_elements()
  {
    return curr_size;
  }

  void enum_buf()
  {
    fprintf(stdout, "ENUMERATING BUFFER\n");
    for( size_t x=0; x<curr_size; ++x)
      {
	fprintf(stdout, "(%f %f)", get_element(x).x, get_element(x).y );
      }
    fprintf(stdout, "\n");
  }
  
  
  T get_element( const size_t& n )
  {
    size_t trueval = (head + n) % max_buf_size;
    if( trueval >= buf.size() )
      {
	fprintf(stderr, "wtf, get_elem trueval > bufsize (%ld vs %ld)\n", trueval, buf.size() );
	exit(1);
      }
    
    return buf[trueval];
  }
  
  void add_element( const T& newelem )
  {
    if( curr_size == max_buf_size )
      {
	size_t tail = (head + curr_size) % max_buf_size; 
	if( tail != head )
	  {
	    fprintf(stderr, "REV; sanity check failed, tail should be head...(tail=%ld, head=%ld)\n", tail, head);
	    exit(1);
	  }
	
	buf[tail] = newelem;
	
	head = (head+1)% max_buf_size;
      }
    else if( curr_size < max_buf_size )
      {
	size_t tail = (head + curr_size) % max_buf_size; 
	buf[tail] = newelem;
	
	++curr_size; 
      }
    else
      {
	fprintf(stderr, "REV: error, curr_size > max_buf_size\n");
	exit(1);
      }
  }
};



struct roc_computer
{
  
  float minthresh;
  float maxthresh;
  size_t nthreshbins;

  size_t tp_fp_head; 
  size_t tp_fp_tail;
  size_t stored_past_tail;
  size_t comput_jump; 
  size_t window_length;

  float curr_auc_roc;
  
  std::vector<float> tps;
  std::vector<float> fps;
  
  std::vector<float> sumtps;
  std::vector<float> sumfps;

  size_t sumqueue_maxsize;
  std::vector<float> firstsumtp;
  std::deque<std::vector<float>> sumqueuetp;
  std::vector<float> firstsumfp;
  std::deque<std::vector<float>> sumqueuefp;
  size_t represents;

  std::deque<size_t> failperjump;
  size_t failures_in_last_jump;
  size_t failures_in_last_window;
  
  roc_computer()
  {
    curr_auc_roc=0;
  }

  roc_computer( const float& mint, const float& maxt, const size_t& nbins, const size_t& compj, const size_t& window_l )
    : minthresh(mint), maxthresh(maxt), nthreshbins(nbins), comput_jump(compj), window_length(window_l)
  {
    tp_fp_head=0;
    tp_fp_tail=0;
    stored_past_tail=0;
    tps.resize( nthreshbins * (window_length + comput_jump) , 0.0);
    fps.resize( nthreshbins * (window_length + comput_jump) , 0.0);
    sumtps.resize( nthreshbins , 0.0 );
    sumfps.resize( nthreshbins , 0.0 );
    curr_auc_roc=-1;
    sumqueue_maxsize = window_length / comput_jump;
    firstsumtp.resize(nbins, 0.0);
    firstsumfp.resize(nbins, 0.0);
    represents=0;
    failures_in_last_window=0;
    failures_in_last_jump=0;
    
  }
  
  void init ( const float& mint, const float& maxt, const size_t& nbins, const size_t& compj, const size_t& window_l )
  {
    minthresh = mint;
    maxthresh = maxt;
    nthreshbins = nbins;
    comput_jump = compj;
    window_length = window_l;
    tp_fp_head=0;
    tp_fp_tail=0;
    stored_past_tail=0;
    curr_auc_roc=-1;
    sumqueue_maxsize = window_length / comput_jump;
    firstsumtp.resize(nbins, 0.0);
    firstsumfp.resize(nbins, 0.0);
    represents=0;
    failures_in_last_window=0;
    failures_in_last_jump=0;
    
        
    tps.resize( nthreshbins * (window_length + comput_jump) , 0.0);
    fprintf(stdout, "Setting TP size to nthresh %ld * (wl %ld + compj %ld) = %ld\n", nthreshbins, window_length, comput_jump, tps.size() );
    
    fps.resize( nthreshbins * (window_length + comput_jump) , 0.0);
    sumtps.resize( nthreshbins , 0.0 );
    sumfps.resize( nthreshbins , 0.0 );

  }

  size_t get_real_offset( const size_t& s ) const
  {
    return (s*nthreshbins) % (tps.size());
  }

  size_t get_gen_offset( const size_t& s ) const
  {
    size_t v = s % (tps.size()/nthreshbins);
    
    return v;
  }
  
  
  std::vector<float> get_generation_from_head( const std::vector<float>& vec, const size_t& offset )
  {
    size_t begin=get_real_offset( ( tp_fp_head + offset ) );
    size_t end=get_real_offset( ( tp_fp_head + offset+1 ) );

    if( begin >= vec.size() || end >= vec.size() ) 
      {
	fprintf(stderr, "REV: error begin/end > size\n");
	exit(1);
      }
    std::vector<float> ret( std::begin( vec ) + begin, std::begin( vec ) + begin + nthreshbins );
    
    return ret;
  }

  
  void add_unsuccessful_sample()
  {
    ++failures_in_last_jump;
  }

  double get_failure_ratio()
  {
    return ((double)failures_in_last_window / (double)window_length) ;
  }

  std::vector<float>::iterator get_generation_from_head_iter( std::vector<float>& vec, const size_t& offset ) 
  {
    size_t begin=get_real_offset( ( tp_fp_head + offset ) );
    size_t end=get_real_offset( ( tp_fp_head + offset+1 ) );
    
    if( begin >= vec.size() || end >= vec.size() ) 
      {
	fprintf(stderr, "REV: error begin/end > size\n");
	exit(1);
      }
    
    return  std::begin( vec ) + begin;
    
  }

  
  void add_new( std::vector<float>& vec, const std::vector<float>& newguy )
  {
    if(newguy.size() != nthreshbins )
      {
	fprintf(stderr, "REV: error newguy size != nthreshbins\n");
	exit(1);
      }
    
    size_t start = get_real_offset( (tp_fp_tail + stored_past_tail) );
    size_t end = get_real_offset( (tp_fp_tail + stored_past_tail + 1));
    if( start >= vec.size() || end >= vec.size() )
      {
	fprintf(stderr, "REV: error newguy would overlap size of targ vec\n");
	exit(1);
      }

    std::copy( std::begin(newguy), std::end(newguy), std::begin(vec)+start );
  }

  
  size_t compute_tail_minus_head()
  {
    size_t res;
    if( tp_fp_head > tp_fp_tail )
      {
	res= tp_fp_tail + ((tps.size()/nthreshbins)-tp_fp_head);
      }
    else
      {
	res = tp_fp_tail - tp_fp_head;
      }

    return (res); 
  }
  
  void compute_first_sum();
  
  
  
  void recompute_sum();

  
  void new_data_point( const std::vector<float>& newtp, const std::vector<float>& newfp )
  {
    
    add_new( tps, newtp );
    add_new( fps, newfp );
    stored_past_tail += 1;
    
    vec_pointwise_sum( firstsumtp, newtp );
    vec_pointwise_sum( firstsumfp, newfp );
    ++represents;
    
    

    
    if( compute_tail_minus_head() < window_length )
      {
	if( stored_past_tail % comput_jump == 0 )
	  {
	    if( represents != comput_jump )
	      {
		fprintf(stderr, "REV: error, not expected comput jump\n");
		exit(1);
	      }
	    sumqueuetp.push_back( firstsumtp );
	    sumqueuefp.push_back( firstsumfp );
	    std::fill( firstsumtp.begin(), firstsumtp.end(), 0.0 );
	    std::fill( firstsumfp.begin(), firstsumfp.end(), 0.0 );
	    represents=0;
	    failperjump.push_front( failures_in_last_jump );
	    failures_in_last_jump = 0;
	    
	  }
	if( sumqueuetp.size() > sumqueue_maxsize )
	  {
	    fprintf(stderr, "REV: err\n");
	    exit(1);
	  }
	
	if( stored_past_tail == window_length )
	  {
	    
	    
	    compute_first_sum();
	    
	    
	    
	    set_AUC( compute_roc() ); 
	  }
      }
    else
      {
	if( stored_past_tail == comput_jump )
	  {
	    
	    
	    
	    
	    
	    

	    
	    

	    
	    
	    
	    
	    
	    
	    
	    
	    
	    recompute_sum();
	    
	    
	    set_AUC( compute_roc() ); 
	  }
      }
  }
  
  void print_sum_tps_fps()
  {
    fprintf(stdout,  "SUM TPS: ");
    print_vec(sumtps);
    fprintf(stdout,  "SUM FPS: ");
    print_vec(sumfps);
    fprintf(stdout, "\n");
  }

  void print_tps_fps()
  {

    fprintf(stdout, "Head: %ld, Tail %ld, Tail-Head %ld, NPastTail %ld\n", tp_fp_head, tp_fp_tail, compute_tail_minus_head(), stored_past_tail);
    fprintf(stdout, "TPS:\n");
    fprintf(stdout, "HEAD----------------(%ld gens)\n", compute_tail_minus_head());
    for(size_t x=0; x<compute_tail_minus_head(); ++x)
      {
	print_vec( get_generation_from_head( tps, x ) );
      }
    fprintf(stdout, "TAIL----------------\n");
    for(size_t x=compute_tail_minus_head(); x<compute_tail_minus_head()+stored_past_tail; ++x)
      {
	print_vec( get_generation_from_head( tps, x ) );
      }
    fprintf(stdout, "\n");

    fprintf(stdout, "FPS:\n");
    fprintf(stdout, "HEAD----------------\n");
    for(size_t x=0; x<compute_tail_minus_head(); ++x)
      {
	print_vec( get_generation_from_head( fps, x ) );
      }
    fprintf(stdout, "TAIL----------------\n");
    for(size_t x=compute_tail_minus_head(); x<compute_tail_minus_head()+stored_past_tail; ++x)
      {
	print_vec( get_generation_from_head( fps, x ) );
      }
    fprintf(stdout, "\n");

  }

  void new_data_point_from_data( const std::vector<float>& tp, const std::vector<float>& fp )
  {
    
    std::vector<float> newtp = compute_rate(tp);
    
    std::vector<float> newfp = compute_rate(fp);
    
    new_data_point(newtp, newfp);
    
  }
  
  std::vector<float> compute_rate( const std::vector<float>& input );

  std::vector<float> compute_avg_fp();
  std::vector<float> compute_avg_tp();


  void set_AUC( const float& f )
  {
    
    curr_auc_roc = f;
  }
  
  float get_AUC()
  {
    return curr_auc_roc;
  }
  
  
  float compute_roc()
  {
    std::vector<float> avgtp = compute_avg_tp(); 
    std::vector<float> avgfp = compute_avg_fp();
    float auc = AUC_from_plot( avgtp, avgfp );
    failperjump.push_front( failures_in_last_jump );
    failures_in_last_window=0;
    for( auto i : failperjump )
      {
	failures_in_last_window += i;
      }
    failures_in_last_jump=0;
    failperjump.pop_back();
    return auc;
    
  }


    
};


 


float get_normalized_sal_value( const int& xpix, const int& ypix, const size_t& gauss_wid_pix, const cv::Mat& normed_frame );


size_t get_random_idx( const size_t& size ,  std::mt19937& gen );

cv::Mat normalize_map( const cv::Mat& myframe );


struct roc_looking_data
{
  rotating_buffer<eyepos> buf;
  size_t N_random_fp; 
  size_t pixel_gauss_wid;
  double MIN_N_MULT;
  
  std::mt19937 gen;
  
  roc_looking_data()
  {
    N_random_fp=200;
    buf.init(N_random_fp*5);
    pixel_gauss_wid=20;
    init_rand();
    MIN_N_MULT=0.2;
  }

  roc_looking_data(const size_t& n, const size_t& wid, const size_t& bufmaxsize )
  {
    N_random_fp=n;
    buf.init(bufmaxsize); 
    pixel_gauss_wid=wid;
    init_rand();
    MIN_N_MULT=0.2;
  }

  void init_rand()
  {
    std::random_device rd;  
    gen = std::mt19937(rd());
  }
  
  
  bool attempt_roc_computation( roc_computer& rc, const eyepos& neweyepos, const cv::Mat& model )
  {
    
    if( buf.buf.size() == 0 )
      {
	fprintf(stderr, "REV: huh, buf never init\n");
	exit(1);
      }
    else if( neweyepos.x < -1e3 )
      {
	rc.add_unsuccessful_sample();
	return false;
      }
    else if( buf.get_n_elements() > N_random_fp*MIN_N_MULT &&
	     false == model.empty() )
      {
	cv::Mat normed_model = model; 
	
	std::vector<float> fps = fp_results( normed_model );
	std::vector<float> tps = tp_results( normed_model, neweyepos );
	
	rc.new_data_point_from_data( tps, fps );
	
	buf.add_element( neweyepos );
	
	return true;
      }
    else 
      {
	buf.add_element( neweyepos );
	return false;
      }
  }

  std::vector<float> fp_results( const cv::Mat& model )
  {
    
    
     

    
    int mheightpix=model.size().height;
    int mwidthpix=model.size().width;

    if( mheightpix<=0 || mwidthpix<=0 )
      {
	fprintf(stderr, "REV: huh model is not proper image, wid/hei < 0\n");
	exit(1);
      }
    
    std::vector<float> resvec(N_random_fp);
    for(size_t x=0; x<N_random_fp; ++x)
      {
	size_t idx = get_random_idx( buf.get_n_elements(), gen );
	eyepos ep = buf.get_element( idx );

	if( ep.x < 0 || ep.y < 0 || ep.x > 1 || ep.y > 1 )
	  {
	    fprintf(stderr, "REV: Eye pos in question is outside xy min maxes (0,1) (0,1)... %f, %f\n", ep.x, ep.y);
	    exit(1);
	  }
	int xpix = mwidthpix * ep.x;
	int ypix = mheightpix * ep.y;

	
	float res = get_normalized_sal_value( xpix, ypix, pixel_gauss_wid, model );
	resvec[x] = res;
      }

    return resvec;
  }
  
  
  std::vector<float> tp_results( const cv::Mat& model, const eyepos& neweyepos )
  {
    int mheightpix=model.size().height;
    int mwidthpix=model.size().width;
    int xpix = mwidthpix * neweyepos.x;
    int ypix = mheightpix * neweyepos.y;
    
    float res = get_normalized_sal_value( xpix, ypix, pixel_gauss_wid, model );

    std::vector<float> resvec(1, res); 
    return resvec;
  }
  
};
  
  
void test_roc_compu();
