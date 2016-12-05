//********************************************************************************************
/**
 * @file        tcc_vdec_api.c
 * @brief		Decode H264 frame and display image onto screen through overlay driver. 
 * 				This interface contain : 
 *
 * @author      Yusuf.Sha, Telechips Shenzhen Rep.
 * @date        2016/11/08
 */
//********************************************************************************************

#include <mach/tcc_overlay_ioctl.h>
#include <mach/vioc_global.h>

#include "tcc_vpudec_intf.h"
#include "tcc_vdec_api.h"

//#define	DEBUG_MODE
#ifdef	DEBUG_MODE
	#define	DebugPrint( fmt, ... )	printf( "[TCC_VDEC_API](D):"fmt, ##__VA_ARGS__ )
	#define	ErrorPrint( fmt, ... )	printf( "[TCC_VDEC_API](E):"fmt, ##__VA_ARGS__ )
#else
	#define	DebugPrint( fmt, ... )
	#define	ErrorPrint( fmt, ... )	printf( "[TCC_VDEC_API](E):"fmt, ##__VA_ARGS__ )
#endif

// Overlay Driver
#define	OVERLAY_DRIVER	"/dev/overlay"
#define DEC_VIDEO_FORMAT VIOC_IMG_FMT_YUV420IL0
#define OVERLAY_GET_WMIXER_OVP      70
#define OVERLAY_SET_WMIXER_OVP      80

typedef struct _DecodeDate {
	
	int 					OverlayDrv;		//overlay driver handler
	bool					IsViewValid;	//view valid ?
	bool					IsDecoderOpen;	//whether decode is in use ?
	bool					IsConfigured;	//whether be configured ?
	unsigned int			default_ovp;	//set overlay layer
	unsigned int			LCD_width;		//LCD size - width
	unsigned int			LCD_height;		//LCD size - height
	overlay_video_buffer_t	lastinfo;		//backup overlay_info
	overlay_video_buffer_t	cur_info;		//current overlay_info
	
	pthread_mutex_t 		mutex_lock;
} DecodeDate;

static DecodeDate decode_data;




void tcc_vdec_SetViewFlag(int isValid)
{
	pthread_mutex_lock(&(decode_data.mutex_lock));
	
	decode_data.IsViewValid = isValid;
	if(!isValid)
		decode_data.IsConfigured = 0;
	
	pthread_mutex_unlock(&(decode_data.mutex_lock));
}

int tcc_vdec_init(unsigned int width, unsigned int height)
{
	//Init decode_data value
	decode_data.OverlayDrv = -1;
	decode_data.IsViewValid = 1;
	decode_data.IsDecoderOpen = 0;
	decode_data.IsConfigured = 0;
	decode_data.default_ovp = 24;
	decode_data.LCD_width = width;
	decode_data.LCD_height = height;
	
	pthread_mutex_init(&(decode_data.mutex_lock),NULL);
	
	return 0;
}

int tcc_vdec_open(void)
{
	pthread_mutex_lock(&(decode_data.mutex_lock));
	memset( &(decode_data.lastinfo), 0, sizeof(overlay_video_buffer_t));
	memset( &(decode_data.cur_info), 0, sizeof(overlay_video_buffer_t));
	
	//--------init and open vpu------------
	if(decode_data.IsDecoderOpen) {
		ErrorPrint( "Decoder is in use! It will be close!\n" );
		tcc_vpudec_close();
		decode_data.IsDecoderOpen = 0;
		decode_data.IsConfigured = 0;
	}
	if(tcc_vpudec_init(800, 466) < 0)
		ErrorPrint("tcc_vpudec_init error!!");
	else
		decode_data.IsDecoderOpen = 1;
	
	//--------open overlay driver------------
	if( decode_data.OverlayDrv >= 0 ){
		close( decode_data.OverlayDrv );
		decode_data.OverlayDrv = -1;
	}
	decode_data.OverlayDrv = open( OVERLAY_DRIVER, O_RDWR );
	
	if( decode_data.OverlayDrv < 0 ){
		ErrorPrint( "Overlay Driver Open Fail\n" );
	}else{
		/*set overlay to the top layer, otherwise, we overlay maybe covered by UI.
		* This value need set accroding to layer which use in overlay driver
		*/
		unsigned int overlay_ovp=0; 
		//ioctl(decode_data.OverlayDrv, OVERLAY_GET_WMIXER_OVP, &(decode_data.default_ovp)); //backup default_ovp
		ioctl(decode_data.OverlayDrv, OVERLAY_SET_WMIXER_OVP, &overlay_ovp);			 //set the new ovp value
	}
	pthread_mutex_unlock(&(decode_data.mutex_lock));
	
	return 0;
}

int tcc_vdec_close(void)
{
	pthread_mutex_lock(&(decode_data.mutex_lock));
	
	ioctl(decode_data.OverlayDrv, OVERLAY_SET_WMIXER_OVP, &(decode_data.default_ovp));
	
	if( decode_data.OverlayDrv >= 0 ){
		close( decode_data.OverlayDrv );
	}
	decode_data.OverlayDrv = -1;
	decode_data.IsConfigured = 0;
	
	if( decode_data.IsDecoderOpen ){
		tcc_vpudec_close();
	}
	decode_data.IsDecoderOpen = 0;
	decode_data.IsViewValid = 0;
	
	memset( &(decode_data.lastinfo), 0, sizeof(overlay_video_buffer_t) );

	pthread_mutex_unlock(&(decode_data.mutex_lock));
	pthread_mutex_destroy(&(decode_data.mutex_lock));
	
	return 0;
}

int tcc_vdec_process_annexb_header( unsigned char* data, int datalen)
{
	int iret = 0;
	unsigned int outputdata[11] = {0};
	unsigned int inputdata[4] = {0};
	
	inputdata[0] = (unsigned int)data;
	inputdata[1] = (unsigned int)datalen;
	
	pthread_mutex_lock(&(decode_data.mutex_lock));
	
	if(decode_data.IsDecoderOpen == 0){
		ErrorPrint( "Decoder is not opened!\n" );
		pthread_mutex_unlock(&(decode_data.mutex_lock));
		return -1;
	}
	
	iret = tcc_vpudec_decode(inputdata, outputdata);
	if(iret < 0) {
		ErrorPrint("Annexb Header Decode Error!\n");
		pthread_mutex_unlock(&(decode_data.mutex_lock));
		return -1;
	}
	
	pthread_mutex_unlock(&(decode_data.mutex_lock));
	
	return 0;
}

#define MAX(a,b) (a>b)?a:b
#define TARGET_WIDTH 1024.00
#define TARGET_HEIGHT 600.00
float TARGET_RATIO = (TARGET_WIDTH/TARGET_HEIGHT);

int tcc_vdec_process( unsigned char* data, int size)
{
	int iret = 0;
	unsigned int inputdata[4] = {0};
	unsigned int outputdata[15] = {0};
	unsigned int screen_width, screen_height;
	//overlay_video_buffer_t info;
	
	pthread_mutex_lock(&(decode_data.mutex_lock));
	
	inputdata[0] = (unsigned int)data;
	inputdata[1] = (unsigned int)size;
	
	screen_width  = decode_data.LCD_width;
	screen_height = decode_data.LCD_height;
	
	if(decode_data.IsDecoderOpen == 0){
		ErrorPrint( "Decoder is not opened!!\n" );
		pthread_mutex_unlock(&(decode_data.mutex_lock));
		return -1;
	}
	if(decode_data.OverlayDrv < 0) {
		ErrorPrint( "Overlay driver is not opened!!\n" );
		pthread_mutex_unlock(&(decode_data.mutex_lock));
		return -1;
	}		
	
	//////////////  Start to Decode video frame
	iret = tcc_vpudec_decode(inputdata, outputdata);
	
	if( iret < 0 ) {
		//ErrorPrint( "Decode Failed!!\n" );
		pthread_mutex_unlock(&(decode_data.mutex_lock));
		return -1;
	}		
	//////////////  Prepare to Push the video /////////////////////
	if(!decode_data.IsViewValid)
	{
		pthread_mutex_unlock(&(decode_data.mutex_lock));
		return 0;
	}
	
	decode_data.cur_info.cfg.width = outputdata[8];
	decode_data.cur_info.cfg.height = outputdata[9];
	decode_data.cur_info.cfg.format = DEC_VIDEO_FORMAT;
	decode_data.cur_info.cfg.transform = 0;		
	decode_data.cur_info.addr = outputdata[1];		// Y Address;
		
#if !defined(CONFIG_OVERLAY_CROP) && !defined(CONFIG_OVERLAY_SCALE)
	//Set position X
	if(decode_data.cur_info.cfg.width <= screen_width)
	{
		decode_data.cur_info.cfg.sx =(screen_width-decode_data.cur_info.cfg.width)/2;
	}
	else
	{
		ErrorPrint("Image width is more then screen_width, need scaler!\n");
		decode_data.cur_info.cfg.sx = 0;
		decode_data.cur_info.cfg.width = screen_width;
	}
	//Set position Y
	if(decode_data.cur_info.cfg.height <= screen_height)
	{
		decode_data.cur_info.cfg.sy =(screen_height-decode_data.cur_info.cfg.height)/2;
	}
	else
	{
		ErrorPrint("Image height is more then screen_hieght, need scaler!\n");
		decode_data.cur_info.cfg.sy = 0;
		decode_data.cur_info.cfg.height = screen_height;
	}
#endif

#if defined(CONFIG_OVERLAY_CROP)	
	decode_data.cur_info.addr1 = outputdata[2];		// U Address;
	decode_data.cur_info.addr2 = outputdata[3];		// V Address;
	
	//for crop
	decode_data.cur_info.cfg.crop_src.left = 0;
	decode_data.cur_info.cfg.crop_src.top = 0;
	decode_data.cur_info.cfg.crop_src.width = outputdata[8]-outputdata[13];
	decode_data.cur_info.cfg.crop_src.height = outputdata[9]-outputdata[14];
	
	//Set position X
	if(decode_data.cur_info.cfg.crop_src.width <= screen_width)
	{
		decode_data.cur_info.cfg.sx =(screen_width - decode_data.cur_info.cfg.crop_src.width)/2;
	}
	else
	{
		ErrorPrint("Image width is more then screen_width, need scaler!\n");
		decode_data.cur_info.cfg.sx = 0;
		decode_data.cur_info.cfg.crop_src.width = screen_width;
	}
	//Set position Y
	if(decode_data.cur_info.cfg.crop_src.height <= screen_height)
	{
		decode_data.cur_info.cfg.sy =(screen_height - decode_data.cur_info.cfg.crop_src.height)/2;
	}
	else
	{
		ErrorPrint("Image height is more then screen_hieght, need scaler!\n");
		decode_data.cur_info.cfg.sy = 0;
		decode_data.cur_info.cfg.crop_src.height = screen_height;
	}
	DebugPrint("Crop: Pos [%d,%d], (%d,%d) -> (%d x %d)... \n", decode_data.cur_info.cfg.sx, 
																decode_data.cur_info.cfg.sy, 
																decode_data.cur_info.cfg.width, 
																decode_data.cur_info.cfg.height, 
																decode_data.cur_info.cfg.crop_src.width, 
																decode_data.cur_info.cfg.crop_src.height);
	//decode_data.cur_info.cfg.width = decode_data.cur_info.cfg.crop_src.width;
	//decode_data.cur_info.cfg.height = decode_data.cur_info.cfg.crop_src.height;
#endif

#if defined(CONFIG_OVERLAY_SCALE)			
	///for scaler
	float ratio0 = (float)decode_data.cur_info.cfg.width/(float)decode_data.cur_info.cfg.height;
	float ratio1 = (float)decode_data.cur_info.cfg.height/(float)decode_data.cur_info.cfg.width;
	//printf("[Yusuf] ratio0=%.5f, ratio1=%.5f, target_ratio=%.5f...\n",ratio0, ratio1, TARGET_RATIO);
	if((ratio0 >= TARGET_RATIO) || (ratio1 >= TARGET_RATIO)) //ratio is 16:9
	{
		if(decode_data.cur_info.cfg.width > decode_data.cur_info.cfg.height)
		{
			decode_data.cur_info.cfg.dispwidth = TARGET_WIDTH;
			decode_data.cur_info.cfg.dispheight = (TARGET_WIDTH*(float)decode_data.cur_info.cfg.crop_src.height)/(float)decode_data.cur_info.cfg.crop_src.width;
		}else{
			decode_data.cur_info.cfg.dispwidth = (TARGET_HEIGHT*(float)decode_data.cur_info.cfg.crop_src.width)/(float)decode_data.cur_info.cfg.crop_src.height;
			decode_data.cur_info.cfg.dispheight = TARGET_HEIGHT;
		}
	}else{//ratio is 4:3  //FIXME
		if(decode_data.cur_info.cfg.width > decode_data.cur_info.cfg.height)
		{
			decode_data.cur_info.cfg.dispwidth = (screen_height*4)/3;//(unsigned int)(480.00 * (MAX(ratio0,ratio1))); //640;
			decode_data.cur_info.cfg.dispheight = screen_height;
		}else{
			decode_data.cur_info.cfg.dispwidth = (screen_height*3)/4;//(unsigned int)(480.00 / (MAX(ratio0,ratio1))); //360;
			decode_data.cur_info.cfg.dispheight = screen_height;
		}
	}
	decode_data.cur_info.cfg.sx = (screen_width - decode_data.cur_info.cfg.dispwidth)/2;
	decode_data.cur_info.cfg.sy = (screen_height - decode_data.cur_info.cfg.dispheight)/2;
	DebugPrint("Scaler: pos (%d x %d) -- src (%d x %d) -> dst (%d x %d) \n",
												decode_data.cur_info.cfg.sx,
												decode_data.cur_info.cfg.sy,
												decode_data.cur_info.cfg.width, 
												decode_data.cur_info.cfg.height, 
												decode_data.cur_info.cfg.dispwidth, 
												decode_data.cur_info.cfg.dispheight);
#endif
	//----------------------------------------------------------------------
	if((decode_data.lastinfo.cfg.width != decode_data.cur_info.cfg.width) 
	|| (decode_data.lastinfo.cfg.height != decode_data.cur_info.cfg.height))
	{	decode_data.IsConfigured = 0;}

	if( decode_data.IsConfigured == 0 ){
	//if(1) {
		ioctl( decode_data.OverlayDrv, OVERLAY_SET_CONFIGURE, &decode_data.cur_info.cfg );
		decode_data.IsConfigured = 1;
	}
	//////////////  Start to Push the video /////////////////////
	ioctl( decode_data.OverlayDrv, OVERLAY_PUSH_VIDEO_BUFFER, &decode_data.cur_info );

	memcpy( &(decode_data.lastinfo), &(decode_data.cur_info), sizeof(overlay_video_buffer_t) );

	pthread_mutex_unlock(&(decode_data.mutex_lock));

	return 0;
}

