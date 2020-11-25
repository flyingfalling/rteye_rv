#pragma once




























void* rteye_wrapper_init( const std::string& ipaddr, const int port, const bool doscene, const bool doeye, const bool dosound, const bool doeyevid, const int reqwid, const int reqhei, const int eyewid, const int eyehei );

void rteye_wrapper_uninit( void* myptr );

void rteye_wrapper_update( void* myptr, long targ_msec );

cv::Mat rteye_wrapper_get_scene_frame( void* myptr );

long rteye_wrapper_get_scene_frame_idx( void* myptr );

long rteye_wrapper_get_scene_frame_pts( void* myptr );

float rteye_wrapper_get_eyex( void* myptr );

float rteye_wrapper_get_eyey( void* myptr );


void rteye_wrapper_calibrate( void* myptr );


float rteye_wrapper_get_calibx( void* myptr );


float rteye_wrapper_get_caliby( void* myptr );



void rteye_wrapper_update_calib_pos( void* myptr, long targ_msec );
void rteye_wrapper_endcalib( void* myptr );
