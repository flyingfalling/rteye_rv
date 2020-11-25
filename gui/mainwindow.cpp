



#include <unistd.h>
#include <errno.h>

#include <filesystem>


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


extern "C"
{
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
}


#include <thread>



#include <timed_mat.hpp>
#include <loop_condition.hpp>
#include <utils.hpp>
#include <roc_compu.hpp>
#include <Timer.hpp>
#include <ThreadPool.hpp>

#include <librteye2.hpp>

#include <rtconnection.hpp>
#include <tobii_ffmpeg_connection.hpp>


#include <cyclic_updater.hpp>


#include <salmap_rv/include/load_salmap.hpp>


#include "mainwindow.h"
#include "ui_mainwindow.h"










































  
  
  
  
  


int VIDWID = 640; 

QStringList qstr_from_stdstr_list( const std::list<std::string>& l )
{
  QStringList q;
  for( auto iter=l.begin(); iter != l.end(); ++iter )
    {
      
      
      q.push_back( QString::fromStdString( *iter ) );
    }
  return q;
}

QStringList qstr_from_stdstr_vector( const std::vector<std::string>& l )
{
  QStringList q;
  for( auto iter=l.begin(); iter != l.end(); ++iter )
    {
      
      
      q.push_back( QString::fromStdString( *iter ) );
    }
  return q;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
  ui->setupUi(this);

  for (int i=0; i<256; i++)
    {
      colorTable.push_back(qRgb(i,i,i));
    }

  size_t displayworkers=2;
  displaytp = std::make_shared<ThreadPool>( displayworkers );
  
  
  update_time_msec = 40;

  
  
  getupdate_timer_40ms = new QTimer(this);
  prepvids_timer_40ms = new QTimer(this);
  display_timer_40ms = new QTimer(this);
  
  QObject::connect(getupdate_timer_40ms, SIGNAL(timeout()), this, SLOT(getScene()));
  QObject::connect(prepvids_timer_40ms, SIGNAL(timeout()), this, SLOT(prepVids()));
  QObject::connect(display_timer_40ms, SIGNAL(timeout()), this, SLOT(displayVids()));
  
  
  saver_timer_40ms = new QTimer(this);
  salmap_timer_40ms = new QTimer(this);
  QObject::connect(saver_timer_40ms, SIGNAL(timeout()), this, SLOT(saveVids()));
  QObject::connect(salmap_timer_40ms, SIGNAL(timeout()), this, SLOT(computeSalmap()));
  
  double sceneimgwidthpix=VIDWID; 
  double sceneimgdva=90;
  double tau_usec=100000;
  
  
  double dva_per_pix = sceneimgdva/sceneimgwidthpix; 
  uint64_t dt_nsec = 40000000; 
  size_t nworkers = 7;
  rocauc=-1;
  auto tp = std::make_shared<ThreadPool>(nworkers);
  std::string param_fname = "./myparams.params"; 
  sal.init( nworkers, dt_nsec, dva_per_pix, tp, param_fname );
  fprintf(stdout, "Done init defaults in mainwindow...\n");
  
  
  ui->lblSal1->setScaledContents( true );
  ui->lblSal1->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
  
  ui->lblSal2->setScaledContents( true );
  ui->lblSal2->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
  
  ui->lblSal3->setScaledContents( true );
  ui->lblSal3->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
  
  ui->lblSal4->setScaledContents( true );
  ui->lblSal4->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
  
  ui->lblSal5->setScaledContents( true );
  ui->lblSal5->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
  
  ui->lblSal6->setScaledContents( true );
  ui->lblSal6->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
  
  ui->lblEyeScene->setScaledContents( true );
  ui->lblEyeScene->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
  
  fprintf(stdout, "Inserting...\n");
  
  auto smaps = sal.get_avail_maps();
  for( auto& s : smaps )
    {
      fprintf(stdout, "%s\n", s.c_str() );
    }
  
  auto asdf = qstr_from_stdstr_list(smaps);
  ui->salModelComboBox->insertItems( 1 , asdf );
   
  fprintf(stdout, "Done3...\n");
  ui->salModelComboBox->setCurrentIndex(0); 


  alpha = 1.0 - (ui->verticalSlider_alpha->value() / 100.0 );
  
  if( ui->checkBoxToggleTime->isChecked() )
    { times_on_salmaps = true; }
  if( ui->checkBoxToggleNames->isChecked() )
    { names_on_salmaps = true; }
  if( ui->checkBoxToggleEyepos->isChecked() )
    { eyepos_on_salmaps = true; }
  
  ui->btnSaveOnOff->setEnabled(false);
  ui->btnSaliencyOnOff->setEnabled(false);
  ui->btnCalibrate->setEnabled(false);
  
  
  updated_salmap=false;


  rtc=NULL;
  cu=NULL;
  vs=NULL;
  fprintf(stdout, "Finished mainwindow constructor\n");
  
   
}

MainWindow::~MainWindow()
{
  computingSalmap=false;
  saving=false;

  
  salmap_loop = false;
  savevids_loop = false;
  
  saver_timer_40ms->stop();
  salmap_timer_40ms->stop();
  getupdate_timer_40ms->stop();
  prepvids_timer_40ms->stop();
  display_timer_40ms->stop();

  if( savethread.joinable() )
    {
      savethread.join();
    }

  if( salthread.joinable() )
    {
      salthread.join();
    }

  
  fprintf(stdout, "Will stop CU\n");
  if( NULL != cu )
    {
      cu->stop();
    }
  fprintf(stdout, "Will stop RTC\n");
  if( NULL != rtc )
    {
      rtc->stop();
    }
  
  delete ui;
  
  fprintf(stdout, "REV: exiting program since main window was killed\n");
  exit(0); 
}

 







void MainWindow::getScene()
{
  ++getcnt;
  
  
  
  
  updatert.reset();
  
  
  
  int64_t nextupdate = cu->wait_and_copy_newest_data( targ_msec, vidres, mm, audres, taf, eyeres, eyex, eyey, "get_and_display" );
    
    
    
  
  
  if(saving)
    {
      vid_timed_queue.add( mm, targ_msec, vidres );
      aud_timed_queue.add( taf, targ_msec, audres );
      eyepos ep( eyex, eyey );
      eye_timed_queue.add( ep, targ_msec, eyeres );
    }
  
    
  
  targ_msec += update_time_msec;
  
  ui->btnSaliencyOnOff->setEnabled(true);
  
  if( true == calibrating && false == rtc->calibrating )
    {
      calibrated = rtc->calibrated; 
      calibrating = false;
      calibthread.join();
    }

  if( calibrating )
    {
      ui->lbl_CalibrateStatus->setText("CALIBRATING...");
    }
  else
    {
      if( false == calibrated )
	{
	  ui->lbl_CalibrateStatus->setText("NOT CALIBRATED");
	}
      else
	{
	  ui->lbl_CalibrateStatus->setText("CALIBRATED");
	}
    }
  
  if( true == calibrating && true == rtc->calibrating )
    {
      float cx2,cy2;
      bool m2d = rtc->get_calib_pos( cx2, cy2, targ_msec-10 );
      if(true == m2d)
	{
	  cx=cx2;
	  cy=cy2;
	}
    }
    
}

void prettifier( int threadid, displaymat& dm, const cv::Mat& m, const double alpha, const bool drawEye, const int eyeres, const bool drawTime, const bool drawName, const float eyex, const float eyey, const int64_t time, const std::string name )
{
  dm.make_pretty( m, alpha, drawEye, eyeres, drawTime, drawName, eyex, eyey, time, name );
}

void vidadder( int threadid, vid_saver* vs, displaymat& dm, cv::Mat& m, const std::string ext, const double d, const bool lossy )
{
  dm.getMat( m );
  
  vs->add_to_video( dm.selected_model, m, dm.selected_model+ext, d, lossy );
}

void vidadder2( int threadid, vid_saver* vs, const std::string s1, const cv::Mat& m, const std::string s2, const double d, const bool lossy )
{
  vs->add_to_video( s1, m, s2, d, lossy );
}



void MainWindow::displayVids()
{
  ++displaycnt;
  
  
  ui->lblEyeScene->setPixmap( QPixmap::fromImage(putImage(eyeScene)) );
  
  if(true == computingSalmap)
    {
      Timer t;
      ui->lblFps->setText( QString::fromStdString( std::to_string(fps) ) );
      ui->lblROC->setText( QString::fromStdString( std::to_string(rocauc) ) );

      if(updated_salmap)
	{
	  
	  std::vector<std::future<void>> myfutures;
	  myfutures.push_back(
			      displaytp->enqueue( prettifier, std::ref(displaymat1), std::ref(raw), alpha, eyepos_on_salmaps, eyeres, times_on_salmaps, names_on_salmaps, eyex, eyey, cu->currtime_msec, "" )
			      );
	  myfutures.push_back(
			      displaytp->enqueue( prettifier, std::ref(displaymat2), std::ref(raw), alpha, eyepos_on_salmaps, eyeres, times_on_salmaps, names_on_salmaps, eyex, eyey, cu->currtime_msec, "" )
			      );
	  myfutures.push_back(
			      displaytp->enqueue( prettifier, std::ref(displaymat3), std::ref(raw), alpha, eyepos_on_salmaps, eyeres, times_on_salmaps, names_on_salmaps, eyex, eyey, cu->currtime_msec, "" )
			      );
	  myfutures.push_back(
			      displaytp->enqueue( prettifier, std::ref(displaymat4), std::ref(raw), alpha, eyepos_on_salmaps, eyeres, times_on_salmaps, names_on_salmaps, eyex, eyey, cu->currtime_msec, "" )
			      );
	  myfutures.push_back(
			      displaytp->enqueue( prettifier, std::ref(displaymat5), std::ref(raw), alpha, eyepos_on_salmaps, eyeres, times_on_salmaps, names_on_salmaps, eyex, eyey, cu->currtime_msec, "" )
			      );
	  myfutures.push_back(
			      displaytp->enqueue( prettifier, std::ref(displaymat6), std::ref(raw), alpha, eyepos_on_salmaps, eyeres, times_on_salmaps, names_on_salmaps, eyex, eyey, cu->currtime_msec, "" )
			      );

	  for( size_t x=0; x<myfutures.size(); ++x )
	    {
	      myfutures[x].get();
	    }
	  
	  
	   
	  	  
	}
      
      
      
      if( displaymat1.hasprettymat )
	{
	  displaymat1.getMat( dm1 );
	  ui->lblSal1->setPixmap( QPixmap::fromImage(putImage( dm1 )) ); 
	}
      if( displaymat2.hasprettymat )
	{
	  displaymat2.getMat( dm2 );
	  ui->lblSal2->setPixmap( QPixmap::fromImage(putImage( dm2 )) ); 
	}
      if( displaymat3.hasprettymat )
	{
	  displaymat3.getMat( dm3 );
	  ui->lblSal3->setPixmap( QPixmap::fromImage(putImage( dm3 )) ); 
	}
      if( displaymat4.hasprettymat )
	{
	  displaymat4.getMat( dm4 );
	  ui->lblSal4->setPixmap( QPixmap::fromImage(putImage( dm4 )) ); 
	}
      if( displaymat5.hasprettymat )
	{
	  displaymat5.getMat( dm5 );
	  ui->lblSal5->setPixmap( QPixmap::fromImage(putImage( dm5 )) ); 
	}
      if( displaymat6.hasprettymat )
	{
	  displaymat6.getMat( dm6 );
	  ui->lblSal6->setPixmap( QPixmap::fromImage(putImage( dm6 )) ); 
	}
      
      
    }
}





void MainWindow::computeSalmap()
{
  
  
  
  
  ++salmapcnt;
  {
    std::unique_lock<std::mutex> lk( salmap_condvar.mux );
    salmap_condvar.val = true;
  }
  
  
  salmap_condvar.cv.notify_all();
}




void t_computeSalmap( timed_mat& mm, salmap_rv::salmap_loader& sal, const cv::Mat& rawMat, displaymat& displaymat1, displaymat& displaymat2,
		      displaymat& displaymat3, displaymat& displaymat4, displaymat& displaymat5, displaymat& displaymat6, std::mutex& mux, double& fps,
		      bool_condvar& salmap_condvar, loop_condition& loop, loop_condition& updated_salmap, cv::Mat& raw, double& rocauc, const int& eyeres, const float& eyex, const float& eyey, double& failratio )
{
  cv::Mat newraw;
  uint64_t cnt=0;
  rteye2::Timer salmaprt2;

  cv::Mat finmat;
  std::shared_ptr<FeatMapImplInst> finfi;
  FeatMapImplPtr finfp = sal.current().getMapAccessor( "output", "smooth_finavg" );

  
  
  double dpp = sal.current().get_input_pix_per_dva();
  double my_gauss_dva=2.50;
  
  roc_looking_data rocdata( 200, 10, 50000); 
  roc_computer roccomp;
  roccomp.init( 0, 1, 10, 25, 500 ); 

  rteye2::Timer alltime_sec;
  
  while( loop )
    {
      
      rteye2::Timer t2;
      {
	std::unique_lock<std::mutex> lk( salmap_condvar.mux );
	while( false == salmap_condvar.val && true == loop )
	  {
	    salmap_condvar.cv.wait_for( lk, std::chrono::milliseconds( 5 ) ); 
	  }
	salmap_condvar.val = false;
      }
      
      
      
      salmaprt2.reset();
      
      if( (mm.mat).size().width <= 0 )
	{
	  fprintf(stderr, "REV: error size of mat is <= 0\n");
	  exit(1);
	}
      

      {
	std::unique_lock<std::mutex> lk(mux);
	
	rawMat.copyTo(newraw);
      }


      
      sal.current().add_input_copy_realtime( "bgr", newraw, alltime_sec.elapsed() );
  
      rteye2::Timer t;
      updated_salmap = sal.current().update();
#if DEBUGLEVEL > 10
      if( t.elapsed()*1e3 > 35 )
	{
	  fprintf(stderr, "WARNING, salmap took >35msec, [%lf]\n", t.elapsed()*1e3);
	}
#endif
      fps = 1.0 / t.elapsed(); 
      
      

      

      
      
      newraw.copyTo(raw);
      
      displaymat1.update( sal.current() );
      displaymat2.update( sal.current() );
      displaymat3.update( sal.current() );
      displaymat4.update( sal.current() );
      displaymat5.update( sal.current() );
      displaymat6.update( sal.current() );

      
      
      

      
      if( eyeres > 0 )
	{
	  rteye2::Timer roct;
	  
	  bool gotfin = sal.current().mapFromAccessor( finfp, finfi, sal.current().get_time_nsec() - sal.current().get_dt_nsec() );
	  bool success=false;
	  
	  if( gotfin )
	    {
	      rocdata.pixel_gauss_wid = my_gauss_dva * finfi->get_pix_per_dva_wid(); 
	      
	      success = rocdata.attempt_roc_computation(roccomp, eyepos(eyex, eyey), finfi->cpu() );
	    }
	  else
	    {
	      success = rocdata.attempt_roc_computation(roccomp, eyepos(eyex, eyey), cv::Mat() );
	      if( true == success )
		{
		  fprintf(stderr, "REV: big error unexpected, should not be success\n");
		  exit(1);
		}
	    }
	  failratio = roccomp.get_failure_ratio();
	  if( success )
	    {
	      
	      rocauc = roccomp.get_AUC();
	    }
	  if( roct.elapsed()*1e3 > 10 )
	    {
	      fprintf(stderr, "REV: WARNING: ROC computation took %lf msec\n", roct.elapsed()*1e3);
	    }
	}
      
      ++cnt;
    }
}





void MainWindow::prepVids()
{
  
  if( mm.mat.empty() )
    {
      fprintf(stderr, "MAT is EMPTY mm\n");
      exit(1);
    }
  
  
  
  
  if( vidres >= 0 )
    {
      std::unique_lock<std::mutex> lk(mux);
      rawMat = mm.mat; 
      
      
      
      
      
      
      
      cv::resize( rawMat, eyeScene, cv::Size(640, 360) ); 
      add_timing( cu->currtime_msec, eyeScene );
      
      if( eyeres >= 0 )
	{
	  draw_eye_pos( eyeScene, eyex, eyey );
	}
      
      if( calibrating )
	{
	  draw_calib_pos( eyeScene, cx, cy );
	}
    }
}





void MainWindow::saveVids()
{
  
  

  
  
  {
    std::unique_lock<std::mutex> lk( savevids_condvar.mux );
    savevids_condvar.val = true;
  }
  savevids_condvar.cv.notify_all();
  
  
  
}
  

void t_saveVids( vid_saver*& vs, const int32_t& vidres, const int32_t& audres, const int32_t& eyeres, timed_audio_frame& taf, int64_t& oldtafidx, cyclic_updater*& cu, const float& eyex, const float& eyey, const cv::Mat& rawMat, const cv::Mat& eyeScene, displaymat& displaymat1, displaymat& displaymat2, displaymat& displaymat3, displaymat& displaymat4, displaymat& displaymat5, displaymat& displaymat6, std::mutex& mux, const bool& computingSalmap, bool_condvar& savevids_condvar, loop_condition& loop, timed_item_queue<timed_mat>& vq, timed_item_queue<timed_audio_frame>& aq, timed_item_queue<eyepos>& eq, double& rocauc, double& failratio )
{
  uint64_t cnt=0;
  rteye2::Timer savert2;
  size_t nthreads=3;
  ThreadPool tp( nthreads );

  cv::Mat m1, m2, m3, m4, m5, m6;
  cv::Mat myeye;
  std::string ext=".mkv";
  bool lossy=true;
  int fps=25;
  
  

  
  
  
  while( loop )
    {
      
      {
	std::unique_lock<std::mutex> lk( savevids_condvar.mux );
	while( false == savevids_condvar.val && true == loop )
	  {
	    savevids_condvar.cv.wait_for( lk, std::chrono::milliseconds( 5 ) ); 
	  }
	savevids_condvar.val = false;
      }

      if( eyeScene.empty() )
	{
	  fprintf(stdout, "Eye scene is still empty...skipping\n");
	  std::this_thread::sleep_for( std::chrono::milliseconds(10) );
	  continue;
	}
      
      
      
      
      
      
       
  
      
      
      std::vector<std::future<void>> myfutures;
      rteye2::Timer t;
      {
	std::unique_lock<std::mutex> lk(mux);
	
	eyeScene.copyTo(myeye);
	
      }

      
      
      
      std::string s1; 
      std::string s2; 
      bool lossless=false;
      
       
      s1="eyeScene";
      s2=s1+ext;

      
      myfutures.push_back(
			  tp.enqueue( vidadder2, vs, s1, std::ref(myeye), s2, fps, lossy )
			  );
      
      
      
      if( computingSalmap )
	{
	  
	  
	  if( displaymat1.hasprettymat )
	    {
	      displaymat1.getMat( m1 );
	      myfutures.push_back(
				  tp.enqueue( vidadder, vs, std::ref(displaymat1), std::ref(m1), ext, fps, lossy )
				  );
	    }
	  if( displaymat2.hasprettymat )
	    {
	      displaymat2.getMat( m2 );
	      myfutures.push_back(
				  tp.enqueue( vidadder, vs, std::ref(displaymat2), std::ref(m2), ext, fps, lossy )
				  );
	    }
	  if( displaymat3.hasprettymat )
	    {
	      displaymat3.getMat( m3 );
	      myfutures.push_back(
				  tp.enqueue( vidadder, vs, std::ref(displaymat3), std::ref(m3), ext, fps, lossy )
				  );
	    }
	  if( displaymat4.hasprettymat )
	    {
	      displaymat4.getMat( m4 );
	      myfutures.push_back(
				  tp.enqueue( vidadder, vs, std::ref(displaymat4), std::ref(m4), ext, fps, lossy )
				  );
	    }
	  if( displaymat5.hasprettymat )
	    {
	      displaymat5.getMat( m5 );
	      myfutures.push_back(
				  tp.enqueue( vidadder, vs, std::ref(displaymat5), std::ref(m5), ext, fps, lossy )
				  );
	    }
	  if( displaymat6.hasprettymat )
	    {
	      displaymat6.getMat( m6 );
	      myfutures.push_back(
				  tp.enqueue( vidadder, vs, std::ref(displaymat6), std::ref(m6), ext, fps, lossy )
				  );
	    }

	  
	   
	}


      
      
      
      
      
      
      auto vid = vq.get_and_pop_first();
      
      if( vid.legal() ) 
	{
	  
	  if( vid.item.mat.empty() )
	    {
	      fprintf(stderr, "EMPTY MAT contained in vq, exit!\n");
	      exit(1);
	    }
	  else
	    {
	      s1="rawScene";
	      s2=s1+ext;
	      
	      
	      
	      myfutures.push_back(
				  tp.enqueue( vidadder2, vs, s1, std::ref(vid.item.mat), s2, fps, lossless )
				  ); 
	    }
	  
	  
	  
	  if( false == vs->started )
	    {
	      fprintf(stdout, "Adding first eye position (and ROC) (%ld msec)!\n", vid.time );
	      vs->add_to_eyepos( vid.time, -1e12, -1e12, "eyepos.txt" );
	      vs->add_to_roc( vid.time, -1e12, -1, "roc.txt" );
	      vs->started = true;
	    }
	}
      
      else
	{
	  
	  fprintf(stderr, "(%ld msec) : VID QUEUE did not contain an item in it! Didn't get added to queue fast enough (I hope)\n", cu->currtime_msec );
	  
	}

            
      auto aud = aq.get_and_pop_first();
      if( aud.legal() )
	{
	  
	  vs->add_to_audio(aud.item.data, "tobiimic.wav");
	  
	}

      float failratio = 0.0;
      vs->add_to_roc( cu->currtime_msec, rocauc, failratio, "roc.txt" );

      auto eye = eq.get_and_pop_first();
      if( eye.legal() )
	{
	  if( eye.tag > 0 ) 
	    {
	      vs->add_to_eyepos( eye.time, eye.item.x, eye.item.y, "eyepos.txt" );
	    }
	}
      
      
      
      
      
      
      for( size_t x=0; x<myfutures.size(); ++x )
	{
	  myfutures[x].get();
	}

      
      
      

      
      
      ++cnt;
    }

  
  size_t remains = vq.items.size();
  fprintf(stdout, "REV: CONTINUING TO WRITE DATA FILES UNTIL ALL ARE DONE (%ld frames remain)!\n", remains);
  auto vid = vq.get_and_pop_first();
  while( vid.legal() )
    {
      remains = vq.items.size();
      if( remains % 100 == 0 )
	{
	  fprintf(stdout, "[%ld] frames remain to write\n", remains);
	}
      if( vid.item.mat.empty() )
	
	{
	  fprintf(stderr, "EMPTY MAT contained in vq, exit!\n");
	  exit(1);
	}
      else
	{
	  vs->add_to_video( "rawScene", vid.item.mat, "rawScene"+ext, fps, false ); 
	   
	}
      vid = vq.get_and_pop_first();
    }
  
  auto aud = aq.get_and_pop_first();
  while( aud.legal() )
    {
      
      vs->add_to_audio(aud.item.data, "tobiimic.wav");
      
      aud = aq.get_and_pop_first();
    }
  
  auto eye = eq.get_and_pop_first();
  while( eye.legal() )
    {
      if( eye.tag > 0 ) 
	{
	  vs->add_to_eyepos( eye.time, eye.item.x, eye.item.y, "eyepos.txt" );
	}
      eye = eq.get_and_pop_first();
    }
  
  fprintf(stdout, "FINISHED WRITING DATA FILES!\n");
}



QImage MainWindow::putImage(const cv::Mat& mat)
{
  
  if(mat.type()==CV_8UC1)
    {
      
      const uchar *qImageBuffer = (const uchar*)mat.data;
      
      QImage img(qImageBuffer, mat.cols, mat.rows, mat.step, QImage::Format_Indexed8);
      img.setColorTable(colorTable);
      return img;
    }
  
  if(mat.type()==CV_8UC3)
    {
      
      const uchar *qImageBuffer = (const uchar*)mat.data;
      
      QImage img(qImageBuffer, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
      return img.rgbSwapped();
    }
  else
    {
      fprintf(stderr, "REV: can't convert mat to qimage (incorrect CV format, currently only handles CV8UC3 and CV8UC1!)\n");
      
      return QImage();
    }
}


void MainWindow::on_btnCalibrate_clicked()
{
  if(calibrating == false )
    {
      calibrating=true;
      rtc->calibrating=true;
      calibthread = std::thread( &rtconnection::calibrate, rtc );
    }
}

void MainWindow::on_btnSaliencyOnOff_clicked()
{
  if( false == computingSalmap )
    {
      
      computingSalmap = true;
      salmap_loop = true;
      salthread = std::thread( t_computeSalmap, std::ref(mm), std::ref(sal), std::ref(rawMat), std::ref(displaymat1), std::ref(displaymat2), std::ref(displaymat3),
			       std::ref(displaymat4), std::ref(displaymat5), std::ref(displaymat6), std::ref(mux), std::ref(fps), std::ref(salmap_condvar), std::ref(salmap_loop), std::ref(updated_salmap), std::ref(raw), std::ref(rocauc), std::ref(eyeres), std::ref(eyex), std::ref(eyey), std::ref(failratio) );
      salmap_timer_40ms->start(update_time_msec);
      ui->lbl_SalOnOff->setText( "ON" );
      getcnt=0; salmapcnt=0; displaycnt=0; prepcnt=0; savecnt=0;
    }
  else
    {
      computingSalmap = false;
      salmap_loop = false;
      salmap_condvar.cv.notify_all();
      salmap_timer_40ms->stop();
      ui->lbl_SalOnOff->setText( "OFF" );
      salthread.join();
    }
}


void MainWindow::on_btnSaveOnOff_clicked()
{
  if( false == saving )
    {
      if( NULL == vs )
	{
	  ui->lblCurrSavePath->setText( QString( "CLICK THE SET PATH BUTTON before saving!" ) );
	}
      else
	{
	  getcnt=0; salmapcnt=0; displaycnt=0; prepcnt=0; savecnt=0;
	  
	  saving = true;
	  savevids_loop = true;
	  savethread = std::thread( t_saveVids, std::ref(vs), std::ref(vidres), std::ref(audres), std::ref(eyeres), std::ref(taf), std::ref(oldtafidx),
				    std::ref(cu), std::ref(eyex), std::ref(eyey), std::ref(rawMat),
				    std::ref(eyeScene), std::ref(displaymat1), std::ref(displaymat2), std::ref(displaymat3),
				    std::ref(displaymat4), std::ref(displaymat5), std::ref(displaymat6), std::ref(mux), std::ref(computingSalmap),
				    std::ref(savevids_condvar), std::ref(savevids_loop), std::ref(vid_timed_queue), std::ref(aud_timed_queue), std::ref(eye_timed_queue), std::ref( rocauc ), std::ref(failratio) );
	  
	  saver_timer_40ms->start(update_time_msec);
	  ui->lbl_SaveOnOff->setText( "ON" );
	  
	  
	}
    }
  else
    {
      saving = false;
      savevids_loop = false;
      savevids_condvar.cv.notify_all(); 
      saver_timer_40ms->stop();
      ui->lbl_SaveOnOff->setText( "OFF" );
      if( savethread.joinable() )
	{
	  savethread.join(); 
	}
    }
}
#include <cstdio>
#include <ctime>

void MainWindow::on_btnSetSaveLocation_clicked()
{
  if( false == saving )
    {
      ui->btnSaveOnOff->setEnabled(true);
    }
  
  
  if( true == saving )
    {
      saving = false;
      savevids_loop = false;
      savevids_condvar.cv.notify_all(); 
      saver_timer_40ms->stop();
      ui->lbl_SaveOnOff->setText( "OFF" );
      if( savethread.joinable() )
	{
	  savethread.join(); 
	}
    }
  
  QString val=ui->lineSaveName->text();
  savename = val.toStdString();

  if( ui->checkBoxAppendDate->isChecked() )
    {
      std::time_t rawtime;
      std::tm* timeinfo;
      char buffer[80];
      
      std::time(&rawtime);
      timeinfo = std::localtime(&rawtime); 
      
      std::strftime(buffer, 80 ,"%Y-%m-%d-%H-%M-%S", timeinfo);
      std::puts(buffer);
      savename += "-" + std::string(buffer);
      
    }
  
  if( NULL != vs )
    {
      fprintf(stdout, "Deleting old vid saver\n");
      delete vs;
    }
  vs = new vid_saver( savename );
  
  ui->lblCurrSavePath->setText( QString::fromStdString(vs->path) );
}

void MainWindow::on_btnConnect_clicked()
{
  QString newip = ui->lineIPAddr->text();
  ipaddr=newip.toStdString();
  
  if( NULL == cu && NULL == rtc )
    {
      ui->lbl_IPAddrConn->setText("Connecting...wait 5 seconds");
      ui->lbl_IPAddrConn->repaint();      
      bool scene=ui->checkBoxVideo->isChecked();
      bool eye=ui->checkBoxEyeTracking->isChecked();
      bool aud=ui->checkBoxAudio->isChecked();
      
      bool started = false;

      loop.set_true();
      
      ui->btnConnect->setEnabled(false);
      std::string dumpfile=""; 
      
      if( NULL != vs )
	{
	  fprintf(stdout, "VidSaver is set, so I'll go ahead and dump all raw data to same directory (%s)\n", vs->path.c_str() );
	  dumpfile = vs->path + "/";
	}
      int port = 49152;

      bool doeyevid=false;
      rtc = new tobii_ffmpeg_connection( &loop, scene, eye, aud, doeyevid, ipaddr, port, dumpfile );
      
      size_t reqwid = VIDWID; 
      size_t reqhei = 0; 
      
      targ_msec = 0;

      size_t eyewid = 300;
      size_t eyehei = 0;
      
      started = rtc->start(reqwid, reqhei, eyewid, eyehei);
      
      if( true == started )
	{
	  ui->lbl_IPAddrConn->setText( "Connected:" + newip );
	  ui->lbl_IPAddrConn->repaint();
	  uint64_t updatet_msec = 10;
	  int64_t sample_offset_msec = -500; 
	  cu = new cyclic_updater( updatet_msec, rtc, sample_offset_msec );
	  
	  cu->start();

	  
	  getupdate_timer_40ms->start(update_time_msec);
	  prepvids_timer_40ms->start(update_time_msec);
	  display_timer_40ms->start(update_time_msec);

	  ui->btnCalibrate->setEnabled(true);
	  
	}
      else
	{
	  loop.set_false();
	  delete rtc;
	  delete cu;
	  rtc = NULL;
	  cu = NULL;
	  ui->lbl_IPAddrConn->setText( "FAILED TO CONNECT" );
	  fprintf(stderr, "Failed to connect to eye tracker?\n");
	  ui->btnConnect->setEnabled(true);
	}
    }
  else
    {
      fprintf( stdout, "Connect button clicked, but CU/RTC not both null...i.e. turn it off?\n");
      if(saving)
	{
	  saver_timer_40ms->stop();
	  saving=false;
	}
      if(computingSalmap)
	{
	  salmap_timer_40ms->stop();
	  computingSalmap=false;
	}

      getupdate_timer_40ms->stop();
      prepvids_timer_40ms->stop();
      display_timer_40ms->stop();

      fprintf(stdout, "This does not work well, so exiting...\n");
      ui->lbl_IPAddrConn->setText( "STOPPED (Exiting)" );
      exit(1);

      if( cu )
	{
	  cu->stop();
	}
      if( rtc )
	{
	  rtc->stop();
	}
      
      if(rtc)
	{
	  delete rtc;
	}
      if(cu)
	{
	  delete cu;
	}
      if(vs)
	{
	  delete vs;
	}
      rtc=NULL;
      cu=NULL;
      
      ui->btnConnect->setEnabled(true);
    }
}




void MainWindow::on_salModelComboBox_currentIndexChanged(const QString &arg1)
{
  
  
  

  updated_salmap=false;

  sal.select_salmap( arg1.toStdString() );
  
  displaymat1.reset();
  displaymat2.reset();
  displaymat3.reset();
  displaymat4.reset();
  displaymat5.reset();
  displaymat6.reset();

  cbs1=0; 
  cbs2=0; 
  cbs3=0; 
  cbs4=0; 
  cbs5=0; 
  cbs6=0; 


  if(ui->checkBoxSal1->isChecked())
    {
      displaymat1.pretty = true;
    }
  else
    {
      displaymat1.pretty = false;
    }
    
  if(ui->checkBoxSal2->isChecked())
    {
      displaymat2.pretty = true;
    }
  else
    {
      displaymat2.pretty = false;
    }
  
  if(ui->checkBoxSal3->isChecked())
    {
      displaymat3.pretty = true;
    }
  else
    {
      displaymat3.pretty = false;
    }
  
  if(ui->checkBoxSal4->isChecked())
    {
      displaymat4.pretty = true;
    }
  else
    {
      displaymat4.pretty = false;
    }
  
  if(ui->checkBoxSal5->isChecked())
    {
      displaymat5.pretty = true;
    }
  else
    {
      displaymat5.pretty = false;
    }
  
  if(ui->checkBoxSal6->isChecked())
    {
      displaymat6.pretty = true;
    }
  else
    {
      displaymat6.pretty = false;
    }
  
  
  
  ui->sal1CBox->clear();
  ui->sal2CBox->clear();
  ui->sal3CBox->clear();
  ui->sal4CBox->clear();
  ui->sal5CBox->clear();
  ui->sal6CBox->clear();

  sal.current().list_filters_and_descs( avail_filters, avail_descs );
  
  
  ui->sal1CBox->insertItem( -1, "" );
  ui->sal2CBox->insertItem( -1, "" );
  ui->sal3CBox->insertItem( -1, "" );
  ui->sal4CBox->insertItem( -1, "" );
  ui->sal5CBox->insertItem( -1, "" );
  ui->sal6CBox->insertItem( -1, "" );

  
  
  
  
  
  
  
  ui->sal1CBox->insertItems( 1, qstr_from_stdstr_vector(avail_descs) );
  ui->sal2CBox->insertItems( 1, qstr_from_stdstr_vector(avail_descs) );
  ui->sal3CBox->insertItems( 1, qstr_from_stdstr_vector(avail_descs) );
  ui->sal4CBox->insertItems( 1, qstr_from_stdstr_vector(avail_descs) );
  ui->sal5CBox->insertItems( 1, qstr_from_stdstr_vector(avail_descs) );
  ui->sal6CBox->insertItems( 1, qstr_from_stdstr_vector(avail_descs) );

  
  
  
  ui->sal1CBox->setCurrentIndex(-1);
  ui->sal2CBox->setCurrentIndex(-1);
  ui->sal3CBox->setCurrentIndex(-1);
  ui->sal4CBox->setCurrentIndex(-1);
  ui->sal5CBox->setCurrentIndex(-1);
  ui->sal6CBox->setCurrentIndex(-1);

  
  ui->sal1CBox->setCurrentIndex(1);
  ui->sal2CBox->setCurrentIndex(2);
  ui->sal3CBox->setCurrentIndex(3);
  ui->sal4CBox->setCurrentIndex(4);
  ui->sal5CBox->setCurrentIndex(5);
  ui->sal6CBox->setCurrentIndex(6);
  
    
}

 

void MainWindow::on_checkBoxSal1_stateChanged(int arg1)
{
  
  if( Qt::Checked == arg1 )
    {
      displaymat1.pretty = true;
    }
  else
    {
      displaymat1.pretty = false;
    }
}

void MainWindow::on_checkBoxSal4_stateChanged(int arg1)
{
  if( Qt::Checked == arg1 )
    {
      displaymat4.pretty = true;
    }
  else
    {
      displaymat4.pretty = false;
    }
}

void MainWindow::on_checkBoxSal2_stateChanged(int arg1)
{
  if( Qt::Checked == arg1 )
    {
      displaymat2.pretty = true;
    }
  else
    {
      displaymat2.pretty = false;
    }
}

void MainWindow::on_checkBoxSal3_stateChanged(int arg1)
{
  if( Qt::Checked == arg1 )
    {
      displaymat3.pretty = true;
    }
  else
    {
      displaymat3.pretty = false;
    }
}

void MainWindow::on_checkBoxSal5_stateChanged(int arg1)
{
  if( Qt::Checked == arg1 )
    {
      displaymat5.pretty = true;
    }
  else
    {
      displaymat5.pretty = false;
    }
}

void MainWindow::on_checkBoxSal6_stateChanged(int arg1)
{
  if( Qt::Checked == arg1 )
    {
      displaymat6.pretty = true;
    }
  else
    {
      displaymat6.pretty = false;
    }
}


void MainWindow::on_btnConnectToFile_clicked()
{
  fromFileRawName = QFileDialog::getOpenFileName( this, "Choose video file", "", "All Files (*)" );
  
  
  if( fromFileRawName.isEmpty() )
    {
      fprintf(stdout, "File name was empty, i.e. user cancelled the selection.\n");
      ui->btnStartFromFile->setEnabled(false);
      return;
    }
  else
    {
      fprintf(stdout, "Set fromFileRawName to [%s]\n", fromFileRawName.toStdString().c_str() );
      ui->btnStartFromFile->setEnabled(true);
      return;
    }
}



void MainWindow::on_sal1CBox_currentIndexChanged(int index)
{
  if( updated_salmap && cbs1 < 1 )
    {
      sal.current().list_filters_and_descs( avail_filters, avail_descs );
      cbs1 = 1; 
      ui->sal1CBox->clear();
      ui->sal1CBox->insertItem( -1, "" );
      ui->sal1CBox->insertItems( 1, qstr_from_stdstr_vector( avail_descs ) );
      ui->sal1CBox->setCurrentIndex(-1); 
      ui->sal1CBox->setCurrentIndex(1);
    }
  else
    {
      if(index > 0 )
	{ displaymat1.set_filter( sal.current(), avail_filters[index-1] ); }
    }
}

void MainWindow::on_sal2CBox_currentIndexChanged(int index)
{
  if( updated_salmap && cbs2 < 1 )
    {
      sal.current().list_filters_and_descs( avail_filters, avail_descs );
      cbs2 = 1;
      ui->sal2CBox->clear();
      ui->sal2CBox->insertItem( -1, "" );
      ui->sal2CBox->insertItems( 1, qstr_from_stdstr_vector( avail_descs ) );
      ui->sal2CBox->setCurrentIndex(-1); 
      ui->sal2CBox->setCurrentIndex(1);
    }
  else
    {
      if(index > 0 )
	{ displaymat2.set_filter( sal.current(), avail_filters[index-1] ); }
    }
}

void MainWindow::on_sal3CBox_currentIndexChanged(int index)
{
  if( updated_salmap && cbs3 < 1 )
    {
      sal.current().list_filters_and_descs( avail_filters, avail_descs );
      cbs3 = 1;
      ui->sal3CBox->clear();
      ui->sal3CBox->insertItem( -1, "" );
      ui->sal3CBox->insertItems( 1, qstr_from_stdstr_vector( avail_descs ) );
      ui->sal3CBox->setCurrentIndex(-1); 
      ui->sal3CBox->setCurrentIndex(1);
    }
  else
    {
      if(index > 0 )
	{ displaymat3.set_filter( sal.current(), avail_filters[index-1] ); }
    }
}

void MainWindow::on_sal4CBox_currentIndexChanged(int index)
{
  if( updated_salmap && cbs4 < 1 )
    {
      sal.current().list_filters_and_descs( avail_filters, avail_descs );
      cbs4 = 1;
      ui->sal4CBox->clear();
      ui->sal4CBox->insertItem( -1, "" );
      ui->sal4CBox->insertItems( 1, qstr_from_stdstr_vector( avail_descs ) );
      ui->sal4CBox->setCurrentIndex(-1); 
      ui->sal4CBox->setCurrentIndex(1);
    }
  else
    {
      if(index > 0 )
	{ displaymat4.set_filter( sal.current(), avail_filters[index-1] ); }
    }
}

void MainWindow::on_sal5CBox_currentIndexChanged(int index)
{
  if( updated_salmap && cbs5 < 1 )
    {
      sal.current().list_filters_and_descs( avail_filters, avail_descs );
      cbs5 = 1;
      ui->sal5CBox->clear();
      ui->sal5CBox->insertItem( -1, "" );
      ui->sal5CBox->insertItems( 1, qstr_from_stdstr_vector( avail_descs ) );
      ui->sal5CBox->setCurrentIndex(-1); 
      ui->sal5CBox->setCurrentIndex(1);
    }
  else
    {
      if(index > 0 )
	{ displaymat5.set_filter( sal.current(), avail_filters[index-1] ); }
    }
}

void MainWindow::on_sal6CBox_currentIndexChanged(int index)
{
  if( updated_salmap && cbs6 < 1 )
    {
      sal.current().list_filters_and_descs( avail_filters, avail_descs );
      cbs6 = 1;
      ui->sal6CBox->clear();
      ui->sal6CBox->insertItem( -1, "" );
      ui->sal6CBox->insertItems( 1, qstr_from_stdstr_vector( avail_descs ) );
      ui->sal6CBox->setCurrentIndex(-1); 
      ui->sal6CBox->setCurrentIndex(1);
    }
  else
    {
      if(index > 0 )
	{ displaymat6.set_filter( sal.current(), avail_filters[index-1] ); }
    }
}



void MainWindow::on_verticalSlider_alpha_sliderMoved(int position)
{
  alpha = 1.0 - ((double)position / 100.0);
}


void MainWindow::on_checkBoxToggleNames_stateChanged(int arg1)
{
  if( Qt::Checked == arg1 )
    {
      names_on_salmaps = true;
    }
  else
    {
      names_on_salmaps = false;
    }
}

void MainWindow::on_checkBoxToggleEyepos_stateChanged(int arg1)
{
    if( Qt::Checked == arg1 )
    {
    eyepos_on_salmaps = true;
    }
    else
    {
        eyepos_on_salmaps = false;
    }

}

void MainWindow::on_checkBoxToggleTime_stateChanged(int arg1)
{
    if( Qt::Checked == arg1 )
    {
      times_on_salmaps = true;
    }
    else
    {
      times_on_salmaps = false;
    }

}

void MainWindow::on_btnStartFromFile_clicked()
{
  if( false == connectedToFile )
    {
      std::string fname=fromFileRawName.toStdString();
      fprintf(stdout, "START from file name [%s]\n", fname.c_str() );
      QString newip = ui->lineIPAddr->text();
      ipaddr=newip.toStdString();
      
      bool scene=ui->checkBoxVideo->isChecked();
      bool eye=ui->checkBoxEyeTracking->isChecked();
      bool aud=false; 
      
      fprintf(stdout, "Making new RT connection object...\n");
      loop.set_true();
      if( NULL != rtc )
	{
	  fprintf(stderr, "Making new RTC, but it's not null...\n");
	  exit(1);
	}

      bool doeyevid=false;
      rtc = new tobii_ffmpeg_connection( &loop, scene, eye, aud, doeyevid, ipaddr );
      
      size_t reqwid=VIDWID; 
      size_t reqhei=0;
      size_t eyewid=300;
      size_t eyehei=0;
      bool started = rtc->start_fromvid( fname, reqwid, reqhei, eyewid, eyehei );
      fprintf(stdout, "Starting from file with required width: %ld\n", reqwid );
      targ_msec = 0;
      if( true == started )
	{
	  ui->lbl_ConnFromFile->setText( QString::fromStdString(fname) );
	  
	  uint64_t updatet_msec = 10;
	  int64_t sample_offset_msec = -120; 
	  cu = new cyclic_updater( updatet_msec, rtc, sample_offset_msec );
	  
	  cu->start(); 
	  
	  getupdate_timer_40ms->start(update_time_msec);
	  prepvids_timer_40ms->start(update_time_msec);
	  display_timer_40ms->start(update_time_msec);

	  ui->btnConnectToFile->setEnabled(false);
	  ui->btnStartFromFile->setText("Click to Stop");
	  connectedToFile = true;
	}
      else
	{
	  fprintf(stderr, "REV: failure to start!");
	  loop.set_false();
	  if( rtc )
	    {
	      delete rtc;
	    }
	  if( cu )
	    {
	      delete cu;
	    }
	  rtc = NULL;
	  cu = NULL;
	  ui->btnStartFromFile->setText("Click to Start");
	  ui->btnConnectToFile->setEnabled(true);
	}
    }
  else 
    {
      fprintf(stdout, "Attempting to stop connection to file!\n");

      if( true == saving )
	{
	  saving = false;
	  savevids_loop = false;
	  savevids_condvar.cv.notify_all(); 
	  saver_timer_40ms->stop();
	  ui->lbl_SaveOnOff->setText( "OFF" );
	  savethread.join();
	}
      
      if( true == computingSalmap )
	{
	  computingSalmap = false;
	  salmap_loop = false;
	  salmap_condvar.cv.notify_all();
	  salmap_timer_40ms->stop();
	  ui->lbl_SalOnOff->setText( "OFF" );
	  salthread.join();
	}
      
      


#if DEBUGLEVEL > 10
      fprintf(stdout, "Sanity check 1\n");
      std::vector<double> b(100000000, 3.33); 
      b.resize(200000000);
      fprintf(stdout, "Sanity check 1, WORKED? \n");
      b.resize(0);
#endif
      loop.set_false();

      getupdate_timer_40ms->stop();
      prepvids_timer_40ms->stop();
      display_timer_40ms->stop();


      
      
      if(cu)
	{
	  cu->stop();
	  delete cu;
	  cu = NULL;
	}
      fprintf(stdout, "(BTN START) CU stopped\n");
      
      if( rtc )
	{
	  rtc->stop();
	  delete rtc;
	  rtc = NULL;
	}
      fprintf(stdout, "(BTN START) RTC stopped\n");
      
      
      
      connectedToFile = false;
      ui->btnStartFromFile->setText("Click to Start");
      ui->btnConnectToFile->setEnabled(true);

#if DEBUGLEVEL > 10
      fprintf(stdout, "Sanity check\n");
      std::vector<double> a(100000000, 3.33); 
      a.resize(200000000);
      fprintf(stdout, "Did it work? \n");
#endif
      
    }
  

  
}







void MainWindow::on_btnRecomputeTrial_clicked()
{
  QString recomputefname = QFileDialog::getExistingDirectory(this, "Choose directory containing raw and eye positions files", "/home", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks );

  return;
   
}
