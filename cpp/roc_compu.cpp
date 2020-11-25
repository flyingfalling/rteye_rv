




#include <vector>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <cstring>


#include <random>
#include <opencv2/opencv.hpp>
#include <utils.hpp>

#include <roc_compu.hpp>


float dist( const float& prevx, const float& prevy, const float& newx, const float& newy )
{
  return sqrt( (prevx-newx)*(prevx-newx) + (prevy-newy)*(prevy-newy) );
}




template <typename T>
void print_vec( const std::vector<T>& v1 )
{
  for(size_t x=0; x<v1.size(); ++x)
    {
      std::cout << v1[x] << " ";
    }
  std::cout << std::endl;
}

template <typename T>
void vec_pointwise_sum( std::vector<T>& v1, const std::vector<T>& v2 )
{
  if( v1.size() != v2.size() )
    {
      fprintf(stderr, "REV: vecs not same size\n");
      exit(1);
    }
  
  for(size_t x=0; x<v1.size(); ++x)
    {
      v1[x] += v2[x];
    }
}

template <typename T>
void vec_pointwise_diff( std::vector<T>& v1, const std::vector<T>& v2 )
{
  if( v1.size() != v2.size() )
    {
      fprintf(stderr, "REV: vecs not same size\n");
      exit(1);
    }
  
  for(size_t x=0; x<v1.size(); ++x)
    {
      v1[x] -= v2[x];
    }
}

template <typename T>
void vec_const_div( std::vector<T>& v1, const T& v2 )
{
  
  for(size_t x=0; x<v1.size(); ++x)
    {
      v1[x] /= v2;
    }
}

template <typename T>
void vec_threshold( const std::vector<T>& input, std::vector<T>& output, const T& thresh)
{
  if(input.size() != output.size())
    {
      output.resize(input.size());
    }

  
  for(size_t x=0; x<input.size(); ++x)
    {
      if(input[x] > thresh)
	{
	  output[x] = 1;
	}
      else
	{
	  output[x] = 0;
	}
    }
}

template <typename T>
T vec_sum( const std::vector<T>& input )
{
  T sum=0;
  for(size_t x=0; x<input.size(); ++x)
    {
      sum += input[x];
    }
  return sum;
}


float AUC_from_plot( const std::vector<float>& truepos, const std::vector<float>& falsepos )
{
  float AUC=0;
  
  for(size_t t=1; t<truepos.size(); ++t)
    {
      float EPSIL=1e-4;
      
      if( (truepos[t] > truepos[t-1]+EPSIL)  || (falsepos[t] > falsepos[t-1]+EPSIL) )
	{
	  fprintf(stderr, "REV: massive error, TP/FP rate of ROC is lower for higher FP rate...(Truepos of %ld is %10.10lf, %ld is %10.10lf)(Falsepos of %ld is %10.10lf, %ld is %10.10lf)\n", t, truepos[t], t-1, truepos[t-1], t, falsepos[t], t-1, falsepos[t-1]);
	  exit(1);
	}
      
      
      float width = falsepos[t-1] - falsepos[t];
      if(width < 0)
	{ width=0; }
      float hei_left = truepos[t];
      float hei_diff = truepos[t-1] - truepos[t];
      if( hei_diff < 0 )
	{
	  hei_diff=0;
	}
      float triangle = (width * hei_diff) / 2;
      float rectangle = width * hei_left;
      
      AUC += (rectangle + triangle);
    }

  return AUC;
}



float get_normalized_sal_value( const int& xpix, const int& ypix, const size_t& gauss_wid_pix, const cv::Mat& normed_frame )
{
  float salvalue = 0;

      
  size_t cnt=0;
  
  for(int x=xpix-(int)gauss_wid_pix; x<=xpix+(int)gauss_wid_pix; ++x)
    {
      for(int y=ypix-(int)gauss_wid_pix; y<=ypix+(int)gauss_wid_pix; ++y)
	{
	  float d=dist( x, y, xpix, ypix );
	  if( d <= gauss_wid_pix )
	    {
	      if( x >= 0 && x < normed_frame.size().width && y>=0 && y<normed_frame.size().height )
		{
		  ++cnt;
		  salvalue += normed_frame.at<float>( cv::Point(x,y) );
		}
	    }
	}
    }

  if(cnt == 0)
    {
      fprintf(stdout, "Huh, no size for normed sal val...(gauss wid is %ld, wid=%d hei=%d, x=%d y=%d)\n", gauss_wid_pix, normed_frame.size().width, normed_frame.size().height, xpix,ypix);
      return 0;
    }
  else
    {
      return (salvalue/(float)cnt); 
    }
}


size_t get_random_idx( const size_t& size ,  std::mt19937& gen )
{
  std::uniform_int_distribution<> dis(0, size-1);
  return dis(gen);
}

cv::Mat normalize_map( const cv::Mat& myframe )
{
  cv::Mat targ;
  myframe.convertTo(targ, CV_32FC1, 1.0f/255);
  cv::Mat tmp;
  targ.copyTo(tmp);
  cv::normalize(targ, tmp, 0.0, 1.0, cv::NORM_MINMAX);
  return tmp;
}

void test_roc_compu()
{
  roc_computer rc;
  float minthresh=0, maxthresh=1.0;
  size_t threshbins=3;
  size_t compj = 3;
  size_t window_l = 5;
  rc.init( minthresh, maxthresh, threshbins, compj, window_l );
  
  fprintf(stdout, "NEW DATA\n");
  
  rc.new_data_point_from_data( {0.6}, {0.3, 0.7, 0.5} );
  rc.print_sum_tps_fps();
  rc.print_tps_fps();
  rc.new_data_point_from_data( {0.6}, {0.3, 0.7, 0.5} );
  rc.print_sum_tps_fps();
  rc.print_tps_fps();
  rc.new_data_point_from_data( {0.6}, {0.3, 0.7, 0.5} );
  rc.print_sum_tps_fps();
  rc.print_tps_fps();
  rc.new_data_point_from_data( {0.6}, {0.3, 0.7, 0.5} );
  rc.print_sum_tps_fps();
  rc.print_tps_fps();
  rc.new_data_point_from_data( {0.6}, {0.3, 0.7, 0.5} );
  rc.print_sum_tps_fps();
  rc.print_tps_fps();
  rc.new_data_point_from_data( {0.6}, {0.3, 0.7, 0.5} );
  rc.print_sum_tps_fps();
  rc.print_tps_fps();
  rc.new_data_point_from_data( {0.4}, {0.3, 0.4, 0.5} );
  rc.print_sum_tps_fps();
  rc.print_tps_fps();
  rc.new_data_point_from_data( {0.6}, {0.3, 0.7, 0.5} );
  rc.print_sum_tps_fps();
  rc.print_tps_fps();
  rc.new_data_point_from_data( {0.9}, {0.3, 0.7, 0.5} );
  rc.print_sum_tps_fps();
  rc.print_tps_fps();
  rc.new_data_point_from_data( {0.6}, {0.3, 0.2, 0.5} );
  rc.print_sum_tps_fps();
  rc.print_tps_fps();
  rc.new_data_point_from_data( {0.6}, {0.3, 0.7, 0.5} );
  rc.print_sum_tps_fps();
  rc.print_tps_fps();
  rc.new_data_point_from_data( {0.6}, {0.3, 0.6, 0.5} );
  rc.print_sum_tps_fps();
  rc.print_tps_fps();
  rc.new_data_point_from_data( {0.6}, {0.3, 0.4, 0.5} );
  rc.print_sum_tps_fps();
  rc.print_tps_fps();
  rc.new_data_point_from_data( {0.3}, {0.3, 0.35, 0.5} );
  rc.print_sum_tps_fps();
  rc.print_tps_fps();
  rc.new_data_point_from_data( {0.6}, {0.3, 0.7, 0.5} );
  rc.print_sum_tps_fps();
  rc.print_tps_fps();
  rc.new_data_point_from_data( {0.3}, {0.3, 0.5, 0.1} );
  rc.print_sum_tps_fps();
  rc.print_tps_fps();
  rc.new_data_point_from_data( {0.9}, {0.1, 0.6, 0.5} );
  rc.print_sum_tps_fps();
  rc.print_tps_fps();
  
  rc.new_data_point_from_data( {0.3}, {0.3, 0.45, 0.9} );
  rc.print_sum_tps_fps();
  rc.print_tps_fps();
  rc.new_data_point_from_data( {0.7}, {0.25, 0.8, 0.9} );
  rc.print_sum_tps_fps();
  rc.print_tps_fps();
  
  fprintf( stdout, "AUC: %f\n", rc.compute_roc() );

}



template <typename T>
void vec_pointwise_sum_iter( std::vector<T>& v1, const typename std::vector<T>::iterator v2 )
{
  
  for(size_t x=0; x<v1.size(); ++x)
    {
      v1[x] += *(v2+x);
    }
}

template <typename T>
void vec_pointwise_diff_iter( std::vector<T>& v1, const typename std::vector<T>::iterator v2 )
{
  
  for(size_t x=0; x<v1.size(); ++x)
    {
      v1[x] -= *(v2+x);
    }
}




void roc_computer::compute_first_sum()
{
  if( stored_past_tail != window_length )
    {
      fprintf(stderr, "REV: trying to call compute_first_sum without having window length num samples\n");
      exit(1);
    }
  
  if( sumqueuetp.size() != sumqueue_maxsize || sumqueuefp.size() != sumqueue_maxsize )
    {
      fprintf(stderr, "Comput first sum, not expected size (expected %ld, got %ld)\n", sumqueue_maxsize, sumqueuetp.size() );
      exit(1);
    }

  std::fill( sumtps.begin(), sumtps.end(), 0.0 );
  std::fill( sumfps.begin(), sumfps.end(), 0.0 );
  for(size_t a=0; a<sumqueue_maxsize; ++a)
    {
      vec_pointwise_sum( sumtps, sumqueuetp[a] );
      vec_pointwise_sum( sumfps, sumqueuefp[a] );
    }
  
    
  
  
  
   
  
  
  
  tp_fp_tail = get_gen_offset(tp_fp_head + window_length);
  stored_past_tail = 0;
}

  
  
void roc_computer::recompute_sum()
{
  
   if( represents != comput_jump )
       {
	 fprintf(stderr, "REV: error, not expected comput jump\n");
	 exit(1);
       }
   
   vec_pointwise_sum( sumtps, firstsumtp );
   vec_pointwise_diff( sumtps, sumqueuetp.front() );

   vec_pointwise_sum( sumfps, firstsumfp );
   vec_pointwise_diff( sumfps, sumqueuefp.front() );
   
   sumqueuetp.push_back( firstsumtp );
   sumqueuefp.push_back( firstsumfp );
   sumqueuetp.pop_front();
   sumqueuefp.pop_front();
   std::fill( firstsumtp.begin(), firstsumtp.end(), 0.0 );
   std::fill( firstsumfp.begin(), firstsumfp.end(), 0.0 );
   represents=0;

   if( sumqueuetp.size() != sumqueue_maxsize || sumqueuefp.size() != sumqueue_maxsize )
     {
       fprintf(stderr, "REV: error somewhere...\n");
       exit(1);
     }
     
  
   
  
  tp_fp_head = get_gen_offset( tp_fp_head + comput_jump );
  tp_fp_tail = get_gen_offset( tp_fp_tail + comput_jump );
  stored_past_tail = 0;
}

std::vector<float> roc_computer::compute_avg_tp()
{
  std::vector<float> tmp = sumtps;
  vec_const_div( tmp, (float)window_length );
  return tmp;
}

std::vector<float> roc_computer::compute_avg_fp()
{
  std::vector<float> tmp = sumfps;
  vec_const_div( tmp, (float)window_length );
  return tmp;
}


float sum_thresh( const std::vector<float>& input, const float& thresh )
{
  size_t sum=0;
  for( size_t x=0; x<input.size(); ++x)
    {
      if( input[x] >= thresh )
	{
	  ++sum;
	}
    }
  return (float)sum;
}

std::vector<float> roc_computer::compute_rate( const std::vector<float>& input )
{
  size_t len=input.size();
  if(len < 1)
    {
      fprintf(stderr, "REV: input size of vector for computing FP/TP rate is 0\n");
      exit(1);
    }
  else
    {
      float threshspan=maxthresh-minthresh;
      float threshjump=threshspan/(nthreshbins-1);
      
      
      std::vector<float> sumed( nthreshbins, 0 );
      for(size_t threshi=0; threshi<nthreshbins; ++threshi)
	{
	  float thr=minthresh + (threshi*threshjump); 
	  if( threshi == nthreshbins-1 )
	    {
	      thr = maxthresh + 0.01; 
	    }
	  
	  float res = sum_thresh( input, thr ); 
	  
	  
	  sumed[threshi] = res / input.size();
	}
      return sumed;
    }
}
