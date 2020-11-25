#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#pragma once

#include <QMainWindow>
#include <QImage>
#include <QTime>
#include <QTimer>
#include <QFileDialog>

 


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


#include <loop_condition.hpp>
#include <timed_mat.hpp>
#include <utils.hpp>
#include <roc_compu.hpp>
#include <Timer.hpp>

#include <librteye2.hpp>
#include <rtconnection.hpp>
#include <tobii_ffmpeg_connection.hpp>


#include <cyclic_updater.hpp>



#include <salmap_rv/include/load_salmap.hpp>
#include <salmap_rv/include/displaymat.hpp>
#include <salmap_rv/include/ThreadPool.hpp>




QStringList qstr_from_stdstr_list( const std::list<std::string>& l );





template <typename T>
struct timed_item
{
  int64_t time;
  T item;
  int tag;

  bool legal()
  {
    if( time >= 0 )
      {
	return true;
      }
    else
      {
	return false;
      }
  }


  timed_item<T> clone()
  {
    timed_item<T> newitem;
    newitem.time = time;
    newitem.item = item;
    newitem.tag = tag;
    return newitem;
  }

  timed_item()
  {
    item = T(); 
    time = -6666666;
    tag = -6666666;
  }

  
  timed_item( const T newitem, const int64_t newtime, const int newtag )
  {
    
    time = newtime;
    
    item = newitem;
    
    tag = newtag;
  }

  

  
  bool operator<( const timed_item<T>& other )
  {
    return ( time < other.time );
  }
};






template <typename T>
struct timed_item_queue
{
  std::list<timed_item<T> > items;
  std::mutex mux;
  
  void add( const T& item, const int64_t& time, const int& tag )
  {
    timed_item<T> ti( item, time, tag );
    std::unique_lock<std::mutex> lk(mux);
    
    items.push_back( ti );
    items.sort(); 
  }

  
  
  timed_item<T> get_target( const int64_t& time ) 
  {
    if( items.size() > 0 )
      {
	std::unique_lock<std::mutex> lk(mux);
	
	for( auto iter=items.begin(); iter != items.end(); ++iter )
	  {
	    if( (*iter).time <= time )
	      {
		return (*iter);
		
		
	      }
	  }
	
	
	
	
	
	fprintf(stderr, "GET_TARGET in timed_item_queue, this should never happen!\n");
	return timed_item<T>();
      }
    else
      {
	
	return timed_item<T>();
      }
  }

  
  timed_item<T> get_first()
  {
    if( items.size() > 0 )
      {
	std::unique_lock<std::mutex> lk(mux);
	return *(items.begin());
	
	
      }
    else
      {
	return timed_item<T>();
	
      }
  }
  
  
  timed_item<T> get_and_pop_first( ) 
  {
    if( items.size() > 0 )
      {
	std::unique_lock<std::mutex> lk(mux);
	
	timed_item<T> first = (*(items.begin())); 
	
	items.pop_front();
	
	return first;
	
      }
    else
      {
	
	return timed_item<T>();
	
      }
  }
};



namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:

  
  
  
  
  
  
  
  
  
  

  
  

  
  

  



  
  
  
  
  

  
  
    void getScene();
    void displayVids();
    void prepVids();
    
    
    void computeSalmap();
    void saveVids();
    
    
    
    void on_btnCalibrate_clicked();

    void on_btnSaliencyOnOff_clicked();

    void on_btnSaveOnOff_clicked();
    void on_btnConnect_clicked();
    void on_btnSetSaveLocation_clicked();
    
    void on_salModelComboBox_currentIndexChanged(const QString &arg1);

     

    void on_checkBoxSal1_stateChanged(int arg1);
    void on_checkBoxSal4_stateChanged(int arg1);
    void on_checkBoxSal2_stateChanged(int arg1);
    void on_checkBoxSal3_stateChanged(int arg1);
    void on_checkBoxSal5_stateChanged(int arg1);
    void on_checkBoxSal6_stateChanged(int arg1);

    void on_btnConnectToFile_clicked();

    

    void on_sal1CBox_currentIndexChanged(int index);

    void on_sal2CBox_currentIndexChanged(int index);

    void on_sal3CBox_currentIndexChanged(int index);

    void on_sal6CBox_currentIndexChanged(int index);

    void on_sal4CBox_currentIndexChanged(int index);

    void on_sal5CBox_currentIndexChanged(int index);

    void on_verticalSlider_alpha_sliderMoved(int position);

    void on_checkBoxToggleNames_stateChanged(int arg1);

    void on_checkBoxToggleEyepos_stateChanged(int arg1);

    void on_checkBoxToggleTime_stateChanged(int arg1);

    void on_btnStartFromFile_clicked();

    void on_btnRecomputeTrial_clicked();

private:
    Ui::MainWindow *ui;
    
    QTimer *getupdate_timer_40ms;
    QTimer *prepvids_timer_40ms; 
    QTimer *salmap_timer_40ms;
    QTimer *display_timer_40ms;
    QTimer *saver_timer_40ms;

  rteye2::Timer updatert, prepvidsrt, salmaprt, displayrt, savert;

    
    QVector<QRgb> colorTable;
        
  int64_t update_time_msec=40;
    QImage putImage(const cv::Mat& mat);
  cyclic_updater* cu; 
  rtconnection* rtc;
    int64_t targ_msec=0;

    double alpha=1.0;

    std::shared_ptr<ThreadPool> displaytp;
    cv::Mat dm1,dm2,dm3,dm4,dm5,dm6;

    bool eyepos_on_salmaps;
    bool times_on_salmaps;
    bool names_on_salmaps;

    timed_mat mm;
    timed_audio_frame taf;
    float eyex, eyey;
    float cx, cy;
    int32_t vidres, audres, eyeres;
    
    
  salmap_rv::salmap_loader sal; 
    
    
  loop_condition loop;

    std::thread salthread, savethread;
    
    

  timed_item_queue<timed_mat> vid_timed_queue;
  timed_item_queue<timed_audio_frame> aud_timed_queue;
  timed_item_queue<eyepos> eye_timed_queue;

  
  double rocauc;
  double failratio;
    
    cv::Mat rawMat;
    cv::Mat eyeScene;
    
    displaymat displaymat1;
    displaymat displaymat2;
    displaymat displaymat3;
    displaymat displaymat4;
    displaymat displaymat5;
    displaymat displaymat6;

    size_t cbs1, cbs2, cbs3, cbs4, cbs5, cbs6;
    
    
    
    
    
    std::vector<std::string> avail_filters; 
    std::vector<std::string> avail_outs; 
    std::vector<std::string> avail_descs; 
    
    int64_t oldtafidx=-1;

  vid_saver* vs;

  QString fromFileRawName; 
    
    bool computingSalmap=false;
    bool calibrated=false;
    bool calibrating=false;
    bool saving=false;
  bool connectedToFile=false;
  
    std::string savename="";
    std::string ipaddr="";

    loop_condition updated_salmap; 
    std::thread calibthread;

    std::mutex mux; 

    double fps;

    cv::Mat raw;
    
  uint64_t getcnt, displaycnt, salmapcnt, prepcnt, savecnt;

    loop_condition salmap_loop, savevids_loop;
    bool_condvar salmap_condvar, savevids_condvar;

     
    
};

void t_computeSalmap( timed_mat& mm, salmap_loader& sal, const cv::Mat& rawMat, displaymat& displaymat1, displaymat& displaymat2, displaymat& displaymat3, displaymat& displaymat4, displaymat& displaymat5, displaymat& displaymat6, std::mutex& mux, double& fps, bool_condvar& salmap_condvar, loop_condition& loop, loop_condition& updated_salmap, cv::Mat& raw, double& rocauc, const int& eyeres, const float& eyex, const float& eyey, double& failratio );


void t_saveVids( vid_saver*& vs, const int32_t& vidres, const int32_t& audres, const int32_t& eyeres, timed_audio_frame& taf, int64_t& oldtafidx, cyclic_updater*& cu, const float& eyex, const float& eyey, const cv::Mat& rawMat, const cv::Mat& eyeScene, displaymat& displaymat1, displaymat& displaymat2, displaymat& displaymat3, displaymat& displaymat4, displaymat& displaymat5, displaymat& displaymat6, std::mutex& mux, const bool& computingSalmap, bool_condvar& savevids_condvar, loop_condition& loop , timed_item_queue<timed_mat>& vq, timed_item_queue<timed_audio_frame>& aq, timed_item_queue<eyepos>& eq, double& rocauc, double& failratio );

#endif 
