
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


#define THREAD_WAKEUP_MS 5


void tobii_ffmpeg_connection::eyepos_threadfunct2_parse_update_eyestate( loop_condition& loop, bool_condvar& eyepos2_condvar, buffered_socket& mybufsock, data_pkg_buffer& dpb )
{
  
  fprintf(stdout, "REV: beginning execution of EYEPOS_THREADFUNCT2: parse_and_update_eyestate\n");
  std::vector<char> tmpbuf( mybufsock.buf.buf.size() );
  size_t copied=0;
  while( true == loop )
    {
      {
	std::unique_lock<std::mutex> lk( eyepos2_condvar.mux );
	
	while( false == eyepos2_condvar.val && true == loop )
	  {
	    eyepos2_condvar.cv.wait_for( lk, std::chrono::milliseconds( THREAD_WAKEUP_MS ) );
	  }

	
	std::memcpy( tmpbuf.data(), mybufsock.buf.buf.data(), mybufsock.buf.avail()*sizeof(mybufsock.buf.buf[0]) );
	copied = mybufsock.buf.avail();
	mybufsock.buf.reset();

	eyepos2_condvar.val = false;
	
      }

      eyepos2_condvar.cv.notify_all();

      
      std::string strdat = std::string( tmpbuf.data(), copied );
      
      
      
      std::stringstream ss( strdat );
      std::string jsonstr;
      while( std::getline( ss, jsonstr, '\n' ) )
	{
	  json_to_data_pkg_buffer( jsonstr, dpb ); 
	}
    }
  
}





void tobii_ffmpeg_connection::eyepos_threadfunct1_receive_socket_packets( loop_condition& loop, bool_condvar& eyepos2_condvar, buffered_socket& mybufsock )
{
    
  fprintf(stdout, "REV: beginning execution of EYEPOS_THREADFUNCT1: receive_socket_packets\n");

  double timeout_sec=2; 
  while( true == loop )
    {
      std::this_thread::sleep_for( std::chrono::microseconds(100) );
      
      
      
      
      int32_t got = recvupto_intobuf( mybufsock.socket, mybufsock.mesg_size, mybufsock.buf );
      if( got < 0 ) 
	{
	  fprintf(stdout, "REV: EYE receive packets error, timeout, peer disconnect, etc. in eyepos threadfunct1\n");
	  loop = false;
	  return;
	}

      if( true == loop && mybufsock.buf.avail() >= mybufsock.triggersize )
	{

	  if( !dumper.empty() )
	    {
	      
	      dumper.adddata( (const char*)mybufsock.buf.buf.data(),
			      mybufsock.buf.avail()*sizeof(mybufsock.buf.buf[0]) ); 
	      
	    }
	  
	  
	  {
	    std::unique_lock<std::mutex> lk( eyepos2_condvar.mux );
	    eyepos2_condvar.val = true;
	  }

	  
	  eyepos2_condvar.cv.notify_all();
	  
	  {
	    std::unique_lock<std::mutex> lk( eyepos2_condvar.mux );

	    
	    while( true == eyepos2_condvar.val && true == loop )
	      {
		eyepos2_condvar.cv.wait_for( lk, std::chrono::milliseconds( THREAD_WAKEUP_MS ) );
		
	      }
	  }
	} 
    }
}





void tobii_ffmpeg_connection::scene_threadfunct1_receive_socket_packets( loop_condition& loop, bool_condvar& scene2_condvar, buffered_socket& mybufsock, mpegts_parser& mytsparser )
{
    
  fprintf(stdout, "REV: beginning execution of SCENE_THREADFUNCT1: receive_socket_packets\n");
  
  rteye2::Timer l;
  
  
  double meanframetime=0;
  double meanframetimetau = 20;
  
  rteye2::Timer waiter;

  double timeout_sec=2; 
  double warning_cutoff_sec = 0.10;
  
  while( true == loop )
    {
      std::this_thread::sleep_for( std::chrono::microseconds(100) );
      
      
       
      
      int32_t got = recvupto_intobuf( mybufsock.socket, mybufsock.mesg_size, mybufsock.buf ); 
      
      

      if( got == 0 && waiter.elapsed() > warning_cutoff_sec )
	{
	  fprintf(stdout, "^^^^^^^^^^^^^^^^ Packet receiver thread [%lf] ms spent waiting for new packet ^^^^^^^^^^^^^^^^^^^^ T=(%lf) (REV: this implies an issue with wireless driver or environment)\n", waiter.elapsed()*1000, l.elapsed()); 
	  loop = false;
	  return;
	}
      else if( got > 0 )
	{
	  waiter.reset();
	}
      else if( got < 0 )
	{
	  fprintf(stderr, "REV: SCENE receive bytes error, timeout, or peer disconnected, or buffer is not large enough! Exiting (stopping loop)\n");
	  loop = false;
	  return;
	}
      
      
      
       

       
      
      
      
      if( got && mybufsock.buf.avail() >= mybufsock.triggersize && true == loop )
	{

	  
	  if( !dumper.empty() )
	    {
	      
	      dumper.addscene( (const char*)mybufsock.buf.buf.data(),
			       mybufsock.buf.avail()*sizeof(mybufsock.buf.buf[0] ) );
	    }
	  
	  
	  
	  
	  
	  
	  mytsparser.mygbuf.add( mybufsock.buf.buf, mybufsock.buf.avail() );
	  
	  
	  mybufsock.buf.reset();
	  
	  size_t avail=mytsparser.mygbuf.avail();
      
	  {
	    std::unique_lock<std::mutex> lk( scene2_condvar.mux );
	
	    if( avail >= mytsparser.MIN_THRESH )
	      {
		scene2_condvar.val = true;
	      }
	    else
	      {
		scene2_condvar.val = false;
	      }
	  }
      
	  scene2_condvar.cv.notify_all();
      
	  
	   
	  
	  
	  
	} 
      else
	{
	  if( false == loop )
	    {
	      {
		std::unique_lock<std::mutex> lk( scene2_condvar.mux );
		fprintf(stdout, "(Force) returning scene thread 1\n");
		scene2_condvar.val = true;
	      }
	      scene2_condvar.cv.notify_all();
	      fprintf(stdout, "Scene T1, Finished notifying\n");
	      return;
	    }
	  
	}
    }
  fprintf(stdout, "Exiting scene threadfunct1\n");
}


#define RAW_OUT_ON_PLANAR 0
void printStreamInformation(const AVCodec* codec, const AVCodecContext* codecCtx, int audioStreamIndex)
{
  fprintf(stderr, "Codec: %s\n", codec->long_name);
  if(codec->sample_fmts != NULL) {
    fprintf(stderr, "Supported sample formats: ");
    for(int i = 0; codec->sample_fmts[i] != -1; ++i) {
      fprintf(stderr, "%s", av_get_sample_fmt_name(codec->sample_fmts[i]));
      if(codec->sample_fmts[i+1] != -1) {
	fprintf(stderr, ", ");
      }
    }
    fprintf(stderr, "\n");
  }
  fprintf(stderr, "---------\n");
  fprintf(stderr, "Stream:        %7d\n", audioStreamIndex);
  fprintf(stderr, "Sample Format: %7s\n", av_get_sample_fmt_name(codecCtx->sample_fmt));
  fprintf(stderr, "Sample Rate:   %7d\n", codecCtx->sample_rate);
  fprintf(stderr, "Sample Size:   %7d\n", av_get_bytes_per_sample(codecCtx->sample_fmt));
  fprintf(stderr, "Channels:      %7d\n", codecCtx->channels);
  fprintf(stderr, "Float Output:  %7s\n", !RAW_OUT_ON_PLANAR || av_sample_fmt_is_planar(codecCtx->sample_fmt) ? "yes" : "no");
}





void tobii_ffmpeg_connection::scene_threadfunct2_read_send_receive_mpegts_packets( loop_condition& loop, bool_condvar& scene2_condvar, bool_condvar& vid1_condvar, bool_condvar& aud1_condvar, mpegts_parser& mytsparser )
{
  AVPacket mypkt;
  
  
  fprintf(stdout, "REV: beginning execution of SCENE_THREADFUNCT2: READ_SEND_RECEIVE_packets\n");

  mytsparser.mygbuf.cv = &scene2_condvar;
  mytsparser.mygbuf.loop = &loop;


  
  uint64_t totalframes=0;
  rteye2::Timer totaltime;

  
  
  
  
  while( true == loop )
    {
      rteye2::Timer waiter;
      
      {
	
	std::unique_lock<std::mutex> lk( scene2_condvar.mux );
	
	if( mytsparser.mygbuf.avail() >= mytsparser.MIN_THRESH )
	  {
	    scene2_condvar.val = true;
	  }
	else
	  {
	    scene2_condvar.val = false;
	  }
	
	
	
	
	while( false == scene2_condvar.val && true == loop )
	  {
	    
	    scene2_condvar.cv.wait_for( lk, std::chrono::milliseconds( THREAD_WAKEUP_MS ) );
	    
	    if( mytsparser.mygbuf.avail() >= mytsparser.MIN_THRESH )
	      {
		scene2_condvar.val = true;
	      }
	    else
	      {
		scene2_condvar.val = false;
	      }
	  }
      }
      
      
      
      if( true == mytsparser.isinit )
      
	{
	  
	  int v=0;
	  
	  while( mytsparser.mygbuf.avail() > mytsparser.MIN_THRESH && true == loop )
	    {
	      
	      
	      
	      
	      v = av_read_frame( mytsparser.format_context, &mypkt );
	      
	      
	      if( 0 == v )
		{
		  
		  if( tsparser.vid_stream_id == mypkt.stream_index )
		    {
		      
		      int sent = avcodec_send_packet( mytsparser.vid_codec_context, &mypkt );
		      
		      
		      if( 0 == sent )
			{
			  
			  
			  rteye2::Timer rc;
			  
			  
			  mytsparser.tmp_decoded_frame_buffer_mux.lock();
			  int recv = avcodec_receive_frame( mytsparser.vid_codec_context, mytsparser.tmp_decoded_frame_buffer.back() );
			  mytsparser.tmp_decoded_frame_buffer_mux.unlock();
			  
			  
			  while( recv >= 0 && true == loop )
			    {
			      ++totalframes;
			      fprintf(stdout, "Receiving frame took [%lf] msec  (%ld total frames in %lf seconds = [%lf] fps)\n", rc.elapsed()*1000, totalframes, totaltime.elapsed(), totalframes/totaltime.elapsed() );
			      
			  
			      size_t avail=0;
			      
			      mytsparser.tmp_decoded_frame_buffer_mux.lock();
			      
			      mytsparser.tmp_decoded_frame_buffer.push( av_frame_alloc() ); 
			      av_frame_unref( mytsparser.tmp_decoded_frame_buffer.back() );
			      avail = mytsparser.tmp_decoded_frame_buffer.size();
			      
			      mytsparser.tmp_decoded_frame_buffer_mux.unlock();
			      
			      {
				
				std::unique_lock<std::mutex> lk( vid1_condvar.mux );
				
				if( avail > 1 ) 
				  {
				    vid1_condvar.val = true;
				  }
				else
				  {
				    vid1_condvar.val = false;
				  }
			      }
			      
			      vid1_condvar.cv.notify_all();

			      
			      
			      recv = avcodec_receive_frame( mytsparser.vid_codec_context, mytsparser.tmp_decoded_frame_buffer.back() );
			      rc.reset();
			    }
			  av_free_packet(&mypkt);
			  
			}
		      else 
			{
			  fprintf(stdout, "REV: sent is false (sendpacket false) (possibly think its EOF?)\n");
			}
		      
		    }
		  else if( mytsparser.aud_stream_id == mypkt.stream_index )
		    {
		      
		      if( audio )
			{

			   
			  int sent = avcodec_send_packet( mytsparser.aud_codec_context, &mypkt );
			  if( sent >= 0 )
			    {
			      int  nrecv=0;
			      int recv = avcodec_receive_frame( mytsparser.aud_codec_context, mytsparser.tmp_decoded_aud_frame );
			      while( recv >= 0 && true == loop)
				
				{
				  if(recv > 0 )
				    {
				      fprintf(stdout, "REV: audio recv is %ld\n", recv);
				    }
				  ++nrecv;
				  if( nrecv > 1 )
				    {
				      fprintf(stdout, "Recieved %ld from same audio pkt\n", nrecv);
				    }
				  

				   
				   

				   
				
				  
				  
				  

				  {
				    
				    std::unique_lock<std::mutex> lk( aud1_condvar.mux );
				    
				    aud1_condvar.val = true;
				  }
				  
				  aud1_condvar.cv.notify_all();

				  
				  {
				    
				    std::unique_lock<std::mutex> lk( aud1_condvar.mux );
				    
				    while( true == aud1_condvar.val && true == loop )
				      {
					aud1_condvar.cv.wait_for( lk, std::chrono::milliseconds (THREAD_WAKEUP_MS ) );
				      }
				  } 
				  
				  recv = avcodec_receive_frame( mytsparser.aud_codec_context, mytsparser.tmp_decoded_aud_frame );
				  
				  
				} 
			      av_free_packet(&mypkt);
			    } 
			  
			} 
		    } 
		  else 
		    {
		      fprintf(stderr, "REV: PKT from unknown stream!\n\n\n");
		      av_free_packet(&mypkt);
		      
		    }
		  
		} 
	      
	      
	      else if(  AVERROR(EAGAIN) == v )
		{
		  fprintf(stdout, "No frame to get while streaming mpegts, try again? (REV: return extra back to gbuf?)\n");
		}
	      else if( AVERROR(EINVAL) == v )
		{
		  fprintf(stdout, "Done with mpegts input (REV: this should never happen while streaming?) (REV: return extra back to gbuf?)\n");
		}
	      else
		{
		  
		  char buf[1000];
		  int ret = av_strerror( v, buf, 1000 );
		  if( ret )
		    {
		      fprintf(stderr, "REV: error getting an error rofl\n");
		      exit(1);
		    }
		  fprintf(stderr, "REV: error [%d]  [%s]\n", v, buf );
		  exit(1);
		}
	      
	      
	      
	       
	    } 
	  
	  
	} 
      else
	{
	  if( false == loop )
	    {
	      fprintf(stdout, "(Force) Exiting scene threadfunct2\n");
	      return;
	    }
	  else
	    {
	      
	    }
	  
	  
	  std::this_thread::sleep_for( std::chrono::milliseconds(5) );
	}
      
    } 
  fprintf(stdout, "Exiting scene threadfunct2\n");
}











void tobii_ffmpeg_connection::eyevid_threadfunct1_receive_socket_packets( loop_condition& loop, bool_condvar& eyevid_condvar, buffered_socket& mybufsock, mpegts_parser& mytsparser )
{
    
  fprintf(stdout, "REV: beginning execution of EYEVID_THREADFUNCT1: receive_socket_packets\n");

  rteye2::Timer l;
  
  
  double meanframetime=0;
  double meanframetimetau = 20;
  
  rteye2::Timer waiter;

  double timeout_sec=2; 
  double warning_cutoff_sec = 0.10;
  
  while( true == loop )
    {
      std::this_thread::sleep_for( std::chrono::microseconds(100) );
      
      
       
      
      int32_t got = recvupto_intobuf( mybufsock.socket, mybufsock.mesg_size, mybufsock.buf ); 
      
      

      if( got == 0 && waiter.elapsed() > warning_cutoff_sec )
	{
	  fprintf(stdout, "^^^^^^^^^^^^^^^^ Packet receiver thread [%lf] ms spent waiting for new packet ^^^^^^^^^^^^^^^^^^^^ T=(%lf) (REV: this implies an issue with wireless driver or environment)\n", waiter.elapsed()*1000, l.elapsed()); 
	  loop = false;
	  return;
	}
      else if( got > 0 )
	{
	  waiter.reset();
	}
      else if( got < 0 )
	{
	  fprintf(stderr, "REV: EYE VID receive bytes error, timeout, or peer disconnected, or buffer is not large enough! Exiting (stopping loop)\n");
	  loop = false;
	  return;
	}
      
      
      
       

       
      
      
      
      if( got && mybufsock.buf.avail() >= mybufsock.triggersize && true == loop )
	{

	  
	  if( !dumper.empty() )
	    {
	      
	      dumper.addeye( (const char*)mybufsock.buf.buf.data(),
			       mybufsock.buf.avail()*sizeof(mybufsock.buf.buf[0] ) );
	    }
	  
	  
	  
	  
	  
	  
	  mytsparser.mygbuf.add( mybufsock.buf.buf, mybufsock.buf.avail() );
	  
	  
	  mybufsock.buf.reset();
	  
	  size_t avail=mytsparser.mygbuf.avail();
      
	  {
	    std::unique_lock<std::mutex> lk( eyevid_condvar.mux );
	
	    if( avail >= mytsparser.MIN_THRESH )
	      {
		eyevid_condvar.val = true;
	      }
	    else
	      {
		eyevid_condvar.val = false;
	      }
	  }
      
	  eyevid_condvar.cv.notify_all();
      
	  
	   
	  
	  
	  
	} 
      else
	{
	  if( false == loop )
	    {
	      {
		std::unique_lock<std::mutex> lk( eyevid_condvar.mux );
		fprintf(stdout, "(Force) returning scene thread 1\n");
		eyevid_condvar.val = true;
	      }
	      eyevid_condvar.cv.notify_all();
	      fprintf(stdout, "Scene T1, Finished notifying\n");
	      return;
	    }
	  
	}
    }
  fprintf(stdout, "Exiting eyevid threadfunct1\n");
}





void tobii_ffmpeg_connection::eyevid_threadfunct2_read_send_receive_mpegts_packets( loop_condition& loop, bool_condvar& eyevid_condvar, bool_condvar& eyevid_decode_condvar, mpegts_parser& mytsparser )
{
  AVPacket mypkt;
  
  
  fprintf(stdout, "REV: beginning execution of EYEVID_THREADFUNCT2: READ_SEND_RECEIVE_packets\n");

  mytsparser.mygbuf.cv = &eyevid_condvar;
  mytsparser.mygbuf.loop = &loop;


  
  uint64_t totalframes=0;
  rteye2::Timer totaltime;

  
  
  
  
  while( true == loop )
    {
      rteye2::Timer waiter;
      
      {
	
	std::unique_lock<std::mutex> lk( eyevid_condvar.mux );
	
	if( mytsparser.mygbuf.avail() >= mytsparser.MIN_THRESH )
	  {
	    eyevid_condvar.val = true;
	  }
	else
	  {
	    eyevid_condvar.val = false;
	  }
	
	
	
	
	while( false == eyevid_condvar.val && true == loop )
	  {
	    
	    eyevid_condvar.cv.wait_for( lk, std::chrono::milliseconds( THREAD_WAKEUP_MS ) );
	    
	    if( mytsparser.mygbuf.avail() >= mytsparser.MIN_THRESH )
	      {
		eyevid_condvar.val = true;
	      }
	    else
	      {
		eyevid_condvar.val = false;
	      }
	  }
      }
      
      
      
      if( true == mytsparser.isinit )
      
	{
	  
	  int v=0;
	  
	  while( mytsparser.mygbuf.avail() > mytsparser.MIN_THRESH && true == loop )
	    {
	      
	      
	      
	      
	      v = av_read_frame( mytsparser.format_context, &mypkt );
	      
	      
	      if( 0 == v )
		{
		  
		  if( tsparser.vid_stream_id == mypkt.stream_index )
		    {
		      
		      int sent = avcodec_send_packet( mytsparser.vid_codec_context, &mypkt );
		      
		      
		      if( 0 == sent )
			{
			  
			  
			  rteye2::Timer rc;
			  
			  
			  mytsparser.tmp_decoded_frame_buffer_mux.lock();
			  int recv = avcodec_receive_frame( mytsparser.vid_codec_context, mytsparser.tmp_decoded_frame_buffer.back() );
			  mytsparser.tmp_decoded_frame_buffer_mux.unlock();
			  
			  
			  while( recv >= 0 && true == loop )
			    {
			      ++totalframes;
			      fprintf(stdout, "Receiving frame took [%lf] msec  (%ld total frames in %lf seconds = [%lf] fps)\n", rc.elapsed()*1000, totalframes, totaltime.elapsed(), totalframes/totaltime.elapsed() );
			      
			  
			      size_t avail=0;
			      
			      mytsparser.tmp_decoded_frame_buffer_mux.lock();
			      
			      mytsparser.tmp_decoded_frame_buffer.push( av_frame_alloc() ); 
			      av_frame_unref( mytsparser.tmp_decoded_frame_buffer.back() );
			      avail = mytsparser.tmp_decoded_frame_buffer.size();
			      
			      mytsparser.tmp_decoded_frame_buffer_mux.unlock();
			      
			      {
				
				std::unique_lock<std::mutex> lk( eyevid_decode_condvar.mux );
				
				if( avail > 1 ) 
				  {
				    eyevid_decode_condvar.val = true;
				  }
				else
				  {
				    eyevid_decode_condvar.val = false;
				  }
			      }
			      
			      eyevid_decode_condvar.cv.notify_all();

			      
			      
			      recv = avcodec_receive_frame( mytsparser.vid_codec_context, mytsparser.tmp_decoded_frame_buffer.back() );
			      rc.reset();
			    }
			  av_free_packet(&mypkt);
			  
			}
		      else 
			{
			  fprintf(stdout, "REV: sent is false (sendpacket false) (possibly think its EOF?)\n");
			}
		      
		    }
		  else 
		    {
		      fprintf(stderr, "REV: PKT from unknown stream!\n\n\n");
		      av_free_packet(&mypkt);
		      
		    }
		  
		} 
	      
	      
	      else if(  AVERROR(EAGAIN) == v )
		{
		  fprintf(stdout, "No frame to get while streaming mpegts, try again? (REV: return extra back to gbuf?)\n");
		}
	      else if( AVERROR(EINVAL) == v )
		{
		  fprintf(stdout, "Done with mpegts input (REV: this should never happen while streaming?) (REV: return extra back to gbuf?)\n");
		}
	      else
		{
		  
		  char buf[1000];
		  int ret = av_strerror( v, buf, 1000 );
		  if( ret )
		    {
		      fprintf(stderr, "REV: error getting an error rofl\n");
		      exit(1);
		    }
		  fprintf(stderr, "REV: error [%d]  [%s]\n", v, buf );
		  exit(1);
		}
	      
	      
	      
	       
	    } 
	  
	  
	} 
      else
	{
	  if( false == loop )
	    {
	      fprintf(stdout, "(Force) Exiting eyevid threadfunct2\n");
	      return;
	    }
	  else
	    {
	      
	    }
	  
	  
	  std::this_thread::sleep_for( std::chrono::milliseconds(5) );
	}
      
    } 
  fprintf(stdout, "Exiting eyevid threadfunct2\n");
}













void tobii_ffmpeg_connection::aud_threadfunct1_receive_decode_mpegts_packets( loop_condition& loop, mpegts_parser& mytsparser, bool_condvar& aud1_condvar )
{
  size_t idx=0;

    
  while( loop )
    {
      
      {
	std::unique_lock<std::mutex> lk( aud1_condvar.mux );
	
	while( false == aud1_condvar.val && true == loop )
	  {
	    aud1_condvar.cv.wait_for( lk, std::chrono::milliseconds( THREAD_WAKEUP_MS ) );
	  }
	
	mytsparser.copy_tmp_aud_to_decoded(); 
	      
	aud1_condvar.val  = false;
      }
      
      aud1_condvar.cv.notify_all();
      
      timed_audio_frame taf;
      taf.add_data( mytsparser.decoded_aud_frame, tsparser.aud_codec_context ); 
      taf.idx = idx;
      ++idx;
      
      mytsparser.rafb.lock();
      
       
      mytsparser.rafb.add( taf );
      mytsparser.rafb.unlock();
    }
}







void tobii_ffmpeg_connection::vid_threadfunct1_BGR_conversion( loop_condition& loop, mpegts_parser& mytsparser, bool_condvar& vid1_condvar )
{
  
  fprintf(stdout, "Beginning vid threadfunct1 (BGR_conversion)\n");

  rteye2::Timer totaltime;
  uint64_t totalframes=0;
  
  while( loop )
    {
      
      
      
      
      size_t avail;
      mytsparser.tmp_decoded_frame_buffer_mux.lock();
      avail = mytsparser.tmp_decoded_frame_buffer.size(); 
      mytsparser.tmp_decoded_frame_buffer_mux.unlock();

      

      
      {
	std::unique_lock<std::mutex> lk( vid1_condvar.mux );

	
	if( avail > 1 ) 
	  {
	    vid1_condvar.val = true;
	  }
	else
	  {
	    vid1_condvar.val = false;
	  }

	
	while( false == vid1_condvar.val && true == loop )
	  {
	    
	    vid1_condvar.cv.wait_for( lk, std::chrono::milliseconds( THREAD_WAKEUP_MS ) );
	    
	    
	    mytsparser.tmp_decoded_frame_buffer_mux.lock();
	    avail = mytsparser.tmp_decoded_frame_buffer.size(); 
	    mytsparser.tmp_decoded_frame_buffer_mux.unlock();
	    	
	    if( avail > 1 ) 
	      {
		vid1_condvar.val = true;
	      }
	    else
	      {
		vid1_condvar.val = false;
	      }
	  }
	
      }
      

      if( false == loop )
	{
	  {
	    std::unique_lock<std::mutex> lk( vid1_condvar.mux );
	    vid1_condvar.val = true;
	  }
	  vid1_condvar.cv.notify_all();
	  return;
	}
      
      
      
      
      
      mytsparser.tmp_decoded_frame_buffer_mux.lock();
      avail = mytsparser.tmp_decoded_frame_buffer.size(); 
      mytsparser.tmp_decoded_frame_buffer_mux.unlock();
      
      
      
      
      mytsparser.copy_tmp_to_decoded();

      
      rteye2::Timer bgrt;
      mytsparser.make_av_frame_BGR(mytsparser.dstX, mytsparser.dstY);
      ++totalframes;
      fprintf(stdout, "Made BGR...... (took [%lf] msec)  (note %ld frames over %lf msec == [%lf] fps)\n", bgrt.elapsed()*1000, totalframes, totaltime.elapsed(), totalframes/totaltime.elapsed());
      
      
      
      
      mytsparser._mm.mt = mytsparser.mt;
      mytsparser._mm.mat = mytsparser.decoded_BGR_mat;
      
      
      mytsparser.rmmb.lock();
      mytsparser.rmmb.add( mytsparser._mm );
      mytsparser.rmmb.unlock();
      
      
      
      
    }
  fprintf(stdout, "Exited thread, BGR conversion\n");
  
}


#define SAME_FRAME (0)
#define NEW_FRAME (1)
#define NO_FRAME (-1)

long ts_to_pts( const long& ts )
{
  
  
  long pts=0;
  pts = (ts * 9e4) / 1e6;
  return pts;
}


void tobii_ffmpeg_connection::flush_frames( const long& pts )
{
  tsparser.rmmb.lock();
  int32_t flushed = tsparser.rmmb.flush_all_less_than( pts );
  fprintf(stdout, "Flushed down to %d video frames\n", flushed);
  tsparser.rmmb.unlock();
}

void tobii_ffmpeg_connection::flush_audio_frames( const long& pts )
{
  tsparser.rafb.lock();
  tsparser.rafb.flush_all_less_than( pts );
  tsparser.rafb.unlock();
}

bool tobii_ffmpeg_connection::copy_last_audio_frame(  timed_audio_frame& taf )
{
  tsparser.rafb.lock();
  int32_t idx = tsparser.rafb.get_oldest( taf ); 
  tsparser.rafb.consume_oldest(); 
  tsparser.rafb.unlock();
  if( idx > 0 )
    {
      return true;
    }
  return false;
  
}

int32_t tobii_ffmpeg_connection::copy_last_audio_frame( const long& pts, timed_audio_frame& taf)
{
  int64_t idx = taf.idx;
  int64_t error=0;
  tsparser.rafb.lock();
  
  int32_t myidx = tsparser.rafb.get_straddled( pts, taf, error);
  tsparser.rafb.unlock();

  if( myidx > 0 )
    {
      if( idx == taf.idx )
	{
	  return 0;
	}
      else
	{
	  return 1;
	}
    }
  else
    {
      return -1;
    }
}

int32_t tobii_ffmpeg_connection::copy_last_frame( timed_mat& retmat )
{
  retmat.mt.pts = -1; 
  int64_t retmatidx = retmat.mt.idx;
  fprintf(stderr, "TRYING TO COPY LAST FRAME (one arg, might not exist)\n");
  rteye2::Timer t;
  
  tsparser.rmmb.lock();
  
  
  int32_t idx = tsparser.rmmb.get_newest( retmat );
  tsparser.rmmb.unlock();

  fprintf(stdout, "xx COPIED LAST FRAME (took [%lf] msec)\n", t.elapsed()*1000);
  
  if( idx > 0 )
    {
      if( retmatidx == retmat.mt.idx )
	{
	  return 0;
	}
      else
	{
	  return 1;
	}
    }
  else
    {
      return -1;
    }
}

int32_t tobii_ffmpeg_connection::copy_last_frame( const int64_t& pts, timed_mat& retmat, int64_t& allowed ) 
{
  retmat.mt.pts = -1; 
  int64_t retmatidx = retmat.mt.idx;
  rteye2::Timer t;
  fprintf(stderr, "TRYING TO COPY LAST FRAME  PTS (three arg, might not exist)\n");
  
  tsparser.rmmb.lock();
  int32_t idx = tsparser.rmmb.get_straddled( pts, retmat, allowed );
  tsparser.rmmb.unlock();
  fprintf(stdout, "xx COPIED LAST FRAME  PTS (took [%lf] msec)\n", t.elapsed()*1000);
  if( idx > 0 )
    {
      if( retmatidx == retmat.mt.idx )
	{
	  return 0;
	}
      else
	{
	  return 1;
	}
    }
  else
    {
      return -1; 
    }
}


void tobii_ffmpeg_connection::start_eyetracking()
{
  keepalive_data_thread = std::thread( send_keepalive_loop, datsocket, KA_DATA_MSG, std::ref( *loop ) );

  eyepos_thread1_receive_socket = std::thread( &tobii_ffmpeg_connection::eyepos_threadfunct1_receive_socket_packets, this, std::ref(*loop), std::ref( eye2_cv), std::ref( buf_dat_socket) );

  eyepos_thread2_parse_update_eyestate = std::thread( &tobii_ffmpeg_connection::eyepos_threadfunct2_parse_update_eyestate, this, std::ref(*loop), std::ref( eye2_cv), std::ref( buf_dat_socket ), std::ref( mydpb ) );
  
  
}

void tobii_ffmpeg_connection::start_scenevid()
{
  vid_thread1_BGR_conversion = std::thread( &tobii_ffmpeg_connection::vid_threadfunct1_BGR_conversion, this, std::ref(*loop), std::ref( tsparser ), std::ref( vid1_cv ) );
}

void tobii_ffmpeg_connection::start_audio()
{
  aud_thread1_recv_decode_mpegts = std::thread( &tobii_ffmpeg_connection::aud_threadfunct1_receive_decode_mpegts_packets, this, std::ref(*loop), std::ref( tsparser ), std::ref( aud1_cv ) );
}


void tobii_ffmpeg_connection::simulate_scenecamera( const size_t fps, const std::string& vidfname, loop_condition& loop )
{
  int64_t sleeptime_usec = (1000000 / fps );
  fprintf(stdout, "Will make a vid capture\n");
  cv::VideoCapture cap( vidfname );
  if( !cap.isOpened() )
    {
      fprintf(stderr, "REV: Capture couldn't open [%s]\n", vidfname.c_str() );
      exit(1);
    }
  cv::Mat m;
  size_t idx=0;
  rteye2::Timer t;
  rteye2::Timer t2;
  uint64_t totaltime;
  int64_t nextsleep=sleeptime_usec;
  double extra=sleeptime_usec;

  
  tsparser.pts_timebase_hz_sec = 90e3; 
  
  std::string vidtail;
  std::string dir=get_canonical_dir_of_fname( vidfname, vidtail );
  
  std::string eyefname = dir + "/eyepos.txt";

  eyepos_reader er( eyefname );
  
  
  
  
  

  

  mydpb.isinit = true;
  
  eyepos ep;
  int64_t gidx=0;
  long latency=0;
  
  fprintf(stdout, "Will start loop in scene camera\n");
  
  while( true == loop )
    {
      bool gotframe = cap.read( m );
      if( gotframe && true == loop )
	{
	  
	  mat_timing mt;
	  mt.idx = idx;
	  int64_t timemsec = idx*40;
	  int64_t pts = msec_to_pts_zeroed( timemsec );
	  
	  if( true == er.get_eyepos( timemsec, ep ) )
	    {
	      
	      std::vector<float> gp;
	      gp.push_back( ep.x );
	      gp.push_back( ep.y );
	      
	      
	      int64_t ts = mydpb.rebased_pts_as_ts( pts );
	      
	      gp_pkg mygppkt( ts, gidx, latency, gp );
	      mydpb.add_gp( mygppkt ); 
	      ++gidx;
	      
	    }
	  else
	    {
	      
	    }
	  
	  
	  mt.pts = msec_to_pts_zeroed( timemsec ); 
	  
	  
	  tsparser._mm.mt = mt;

	  
	  
	  cv::Mat m2;
	  
	  if(0 == tsparser.dstX)
	    {
	      fprintf(stderr, "REV: tsparser, dstX not set in fromfile!?\n");
	      exit(1);
	    }
	  if(0 == tsparser.dstY)
	    {
	      double ratio = (double)m.size().height / m.size().width;
	      tsparser.dstY = (tsparser.dstX * ratio) + 0.5;
	    }
	  cv::resize( m, m2, cv::Size(tsparser.dstX, tsparser.dstY) );
	  tsparser._mm.mat = m2; 
	  
	  
	  tsparser.rmmb.lock();
	  tsparser.rmmb.add( tsparser._mm );
	  tsparser.rmmb.unlock();
	  
	  ++idx;

	  
	  
	  
	  
	  int64_t diff=(int64_t)extra - nextsleep; 
	  
	  nextsleep = sleeptime_usec - diff;
	  
	  
	  if( nextsleep < 0 && (idx+1) % 100 == 0 )
	    {
	      fprintf(stderr, "(Simulate scene camera) : Whoa sleeping negative amount...?\n");
	    }
	  std::this_thread::sleep_for( std::chrono::microseconds(nextsleep) );
	  if( (idx+1) % 100 == 0 )
	    {
	      fprintf(stdout, "Vid replay timing check: Total elapsed time %lf  Supposed elapsed time %lf\n", t2.elapsed(), (idx*sleeptime_usec)/1e6 );
	    }
	  
	  extra = t.elapsed()*1e6; 
	  t.reset();
	  
	}
      else
	{
	  fprintf(stdout, "Finished playing video file [%s]\n", vidfname.c_str() );
	  loop = false;
	  return;
	}
    }
  
}

bool tobii_ffmpeg_connection::start_fromvid( const std::string& vidfname, const size_t reqwid, const size_t reqhei , const size_t eye_reqwid, const size_t eye_reqhei)
{
  fprintf(stdout, "Starting from Vid\n");
  
  
  
  
  
  fromfile=true;

  fprintf(stdout, "Will init tsparser\n");
  
  size_t vidbufsize=50; 
  size_t audbufsize=200;

  
  tsparser.init( reqwid, reqhei, vidbufsize, audbufsize );

  if( eyevid )
    {
      eyevid_tsparser.init( eye_reqwid, eye_reqhei, vidbufsize, audbufsize );
      fprintf(stdout, "xx Finished EYEVID TSPARSER INIT\n");
    }
   
  fprintf(stdout, "Was initialized\n");
  size_t fps=25;
  vidfile_thread1_read = std::thread(&tobii_ffmpeg_connection::simulate_scenecamera, this, fps, vidfname, std::ref(*loop) );
  fprintf(stdout, "Spun off vidfile thread1\n");

  return true;
}



bool tobii_ffmpeg_connection::start( const size_t reqwid, const size_t reqhei, const size_t eye_reqwid, const size_t eye_reqhei  )
  {
    fprintf(stdout, "Starting keepalive/receive data (eye pos, and video scene) threads\n");

    int NTRIES=2;
    bool started=false;
    bool started_eyevid = false;
    
    
    
    size_t vidbufsize=50;
    size_t audbufsize=200;
    
    tsparser.init( reqwid, reqhei, vidbufsize, audbufsize );
    fprintf(stdout, "xx Finished TSPARSER INIT\n");
    
    if( eyevid )
      {
	if( eye_reqwid == 0 && eye_reqhei == 0 )
	  {
	    fprintf(stderr, "Hm wtf 0?\n");
	    exit(1);
	  }
	eyevid_tsparser.init( eye_reqwid, eye_reqhei, vidbufsize, audbufsize );
	fprintf(stdout, "xx Finished EYEVID TSPARSER INIT\n");
      }
    
    if( !dumper.empty() )
      {
	fprintf(stdout, "OPENING DUMPER\n");
	dumper.t.reset(); 
	dumper.openfiles();
      }

    fprintf(stdout, "xx Finished DUMPER INIT\n");
    
    while( NTRIES > 0 && ( started == false || (started_eyevid == false && eyevid==true) ) )
      {
	*loop = true;

	fprintf(stdout, "Will attempt to conn to socket...\n");
	conn_socket_ip();

	fprintf(stdout, " xx Done CONN socket\n");
	
	keepalive_scene_thread = std::thread( send_keepalive_loop, vidsocket, KA_VIDEO_MSG, std::ref( *loop ) );

	fprintf(stdout, "Started spinning SCENE VID keepalive\n");
	
	
	
	
	
	std::this_thread::sleep_for( std::chrono::milliseconds(10) ); 
	
	scene_thread1_receive_socket = std::thread( &tobii_ffmpeg_connection::scene_threadfunct1_receive_socket_packets, this, std::ref(*loop), std::ref(scene2_cv), std::ref( buf_vid_socket ), std::ref(tsparser) );
	
	fprintf(stdout, "Started spinning receive socket video\n");


	std::this_thread::sleep_for( std::chrono::milliseconds(10) ); 
	
	scene_thread2_read_send_receive_mpegts = std::thread( &tobii_ffmpeg_connection::scene_threadfunct2_read_send_receive_mpegts_packets, this, std::ref(*loop), std::ref(scene2_cv), std::ref(vid1_cv), std::ref(aud1_cv), std::ref( tsparser ) );

	fprintf(stdout, "Started spinning decoder video\n");



	std::this_thread::sleep_for( std::chrono::milliseconds(10) ); 
	
	
	started = tsparser.init_decoding(); 
	
	fprintf(stdout, "Finished init SCENE VID decoding...\n");


	if( eyevid )
	  {
	    keepalive_eye_thread = std::thread( send_keepalive_loop, eyesocket, KA_EYE_MSG, std::ref( *loop ) );
	    fprintf(stdout, "Started spinning EYE VID keepalive\n");
	    
	
	    
	    eyevid_thread1_receive_socket = std::thread( &tobii_ffmpeg_connection::eyevid_threadfunct1_receive_socket_packets, this, std::ref(*loop), std::ref(eyevid_cv), std::ref( buf_eye_socket ), std::ref(eyevid_tsparser) );
	    fprintf(stdout, "Started spinning receive socket EYE VIDEO\n");

	    std::this_thread::sleep_for( std::chrono::milliseconds(1000) );
	
	    eyevid_thread2_read_send_receive_mpegts = std::thread( &tobii_ffmpeg_connection::eyevid_threadfunct2_read_send_receive_mpegts_packets, this, std::ref(*loop), std::ref(eyevid_cv), std::ref(eyevid_decode_cv), std::ref( eyevid_tsparser ) );
	    
	    fprintf(stdout, "Started spinning EYE VID decoder video\n");
	    
	    
	    
	    started_eyevid = eyevid_tsparser.init_decoding(); 
	    fprintf(stdout, "Finished EYE VID init decoding...\n");
	  }
	
	if( false == started || ( true == eyevid && false == started_eyevid ) )
	  {
	    --NTRIES;
	    *loop = false;
	    fprintf(stderr, "Failed to connect and get enough data librteye2 start(), so will try again. Joining\n");
	    
	    std::this_thread::sleep_for( std::chrono::milliseconds(200) );
	    scene_thread2_read_send_receive_mpegts.join();
	    scene_thread1_receive_socket.join();
	    keepalive_scene_thread.join();
	    
	    if( eyevid )
	      {
		eyevid_thread2_read_send_receive_mpegts.join();
		eyevid_thread1_receive_socket.join();
		keepalive_eye_thread.join();
	      }
	    
	    close_socket_ip();
	    
	    fprintf(stderr, "Joined threads in preparation to restart.\n");
	  }
      }
    
    if( false == started )
      {
	fprintf(stderr, "REV: start() in librteye2, could not get enough data in tsparser init_decoding()\n");
	
	return false;
      }
    
    if( scenevid )
      {
	start_scenevid();
      }
    
    if( eyetracking )
      {
	start_eyetracking();
      }

    
    if( audio )
      {
	start_audio();
      }

    return true;
    
    
    
  }





bool tobii_ffmpeg_connection::zero_timing()
{
  
  timed_mat m;
  
  
  int i = copy_last_frame( m );
  
  
  
  
  if( i >= 0 )
    {
      
      zeroed_pts = m.mt.pts;
      fprintf(stdout, "REV: setting zero time of tobii ffmpeg to [%ld]\n", zeroed_pts );
      return true;
    }
  else
    {
      return false;
      
      
    }
}

rteye2::Timer xxxx;


int32_t tobii_ffmpeg_connection::get_video_frame( timed_mat& retmat, const int64_t& zeroed_time_ms ) 
{
  int64_t targetpts = msec_to_pts_zeroed(zeroed_time_ms);
  
  retmat.mt.pts = -1; 
  int64_t retmatidx = retmat.mt.idx;
  int64_t allowed;
  
  
  
  tsparser.rmmb.lock();
  int32_t idx = tsparser.rmmb.get_straddled( targetpts, retmat, allowed );
  tsparser.rmmb.unlock();
  
  
  
  xxxx.reset();
  if( idx > 0 )
    {
      if( retmatidx == retmat.mt.idx )
	{
	  return 0;
	}
      else
	{
	  return 1;
	}
    }
  else
    {
      return -1;
    }
}







int32_t tobii_ffmpeg_connection::get_audio_frame( timed_audio_frame& rettaf, const int64_t& zeroed_start_time_ms )
{
  if( false == audio )
    {
      return -1;
    }

  int64_t targetpts = msec_to_pts_zeroed( zeroed_start_time_ms );

  int64_t allowed=0;
  tsparser.rafb.lock();
  int32_t idx = tsparser.rafb.get_straddled( targetpts, rettaf, allowed ); 
  allowed=0;
  tsparser.rafb.unlock();

  if( idx > 0 )
    {
      return 1;
    }
  else
    {
      
      return -1;
    }
}



int32_t tobii_ffmpeg_connection::get_audio_frame( timed_audio_frame& rettaf, const int64_t& zeroed_start_time_ms, const int64_t& len_ms ) 
{
  if( false == audio )
    {
      return -1;
    }
  
  int64_t targetpts = msec_to_pts_zeroed( zeroed_start_time_ms );
  int64_t targetendpts = msec_to_pts_zeroed( zeroed_start_time_ms + len_ms ); 

  timed_audio_frame tmptaf;  
  
  
  
  
  
  int64_t allowed=0;
  tsparser.rafb.lock();
  int32_t idx = tsparser.rafb.get_straddled( targetpts, rettaf, allowed ); 
  allowed=0;
  
  int32_t idx2 = tsparser.rafb.get_straddled( targetendpts, tmptaf, allowed); 

  
  
  tsparser.rafb.unlock();

  
  int64_t ptslen = msec_to_pts( len_ms );
  int64_t ptsoffset = targetpts - rettaf.get_time(); 
  
  

  int64_t msec_offset = pts_to_msec( ptsoffset );
  int64_t nsamples = msec_to_audio_samples( len_ms );
  int64_t sampleoffset = msec_to_audio_samples( msec_offset );
  int64_t byteoffset = sampleoffset * tsparser.audio_bytes_per_sample;
  int64_t bytelen = nsamples * tsparser.audio_bytes_per_sample;
  
  std::vector<uint8_t> tmpdat( bytelen );
  
  
  if( rettaf.data.size() <= byteoffset )
    {
      
      rettaf.data.resize( bytelen, 0 ); 
      return 1;
      
      
      
    }

  
  
  int64_t firstavail = rettaf.data.size() - byteoffset; 

  if( firstavail <= 0 )
    {
      
      rettaf.data.resize(bytelen, 0);
      return 1;
    }

  
  if( firstavail >= bytelen )
    {
      
      
      
      std::copy( std::begin(rettaf.data)+byteoffset, std::begin(rettaf.data)+byteoffset+bytelen, std::begin(tmpdat) );
    }
  else
    {
      int64_t tocpy = bytelen - firstavail;
      
      
      
      std::copy(  std::begin(rettaf.data)+byteoffset, std::end(rettaf.data), std::begin(tmpdat) );
      
      if( tocpy <= 0 )
	{
	  fprintf(stderr, "REV: wtf this should be >0 in audio copy...\n");
	  exit(1);
	}
      
      if( idx != idx2 )
	{
	  
	  std::copy(  std::begin(tmptaf.data), std::begin(tmptaf.data)+tocpy,  std::begin(tmpdat)+firstavail);
	}
      else  
	{
	  
	  
	  std::vector<uint8_t> toinsert( tocpy, 0);
	  
	  std::copy(  std::begin(toinsert), std::end(toinsert), std::begin(tmpdat)+firstavail );
	}
    }

  
  
  
  rettaf.data = tmpdat;
  
  
  rettaf.idx = -1;
  rettaf.pts = -1;
  
  
  
  
  if( idx > 0 )
    {
      return 1;
    }
  else
    {
      fprintf(stdout, "There do not exist any audio frames yet?\n");
      return -1;
    }
  
}

int32_t tobii_ffmpeg_connection::get_eyepos( float& x, float& y, const int64_t& zeroed_time_ms ) 
{
  
  if( !eyetracking )
    {
      
      return -1;
    }
  
  int64_t targetpts = msec_to_pts_zeroed(zeroed_time_ms);
  
  bool flushupto=true;
  int64_t allowedoffset=0;
  bool got = mydpb.get_gp_of_pts( targetpts, allowedoffset, x, y, flushupto );


  if( true == got )
    {
      return 1; 
    }
  else
    {
      return -1;
    }
  
}


int32_t tobii_ffmpeg_connection::get_calib_pos( float& x, float& y, const int64_t& zeroed_time_ms ) 
{
  
  if( !eyetracking )
    {
      return -1;
    }

  int64_t targetpts = msec_to_pts_zeroed(zeroed_time_ms);
  bool flushupto=true;
  int64_t allowedoffset=0;
  bool got = mydpb.get_m2d_of_pts( targetpts, allowedoffset, x, y, flushupto );


  if( true == got )
    {
      return 1;
    }
  else
    {
      return -1;
    }
  
}
