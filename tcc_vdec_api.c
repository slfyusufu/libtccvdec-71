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

typedef struct _DecodeDate {
	
	int 					OverlayDrv;		//overlay driver handler
	bool					IsViewValid;	//view valid ?
	bool					IsDecoderOpen;	//whether decode is in use ?
	bool					IsConfigured;	//whether be configured ?
	unsigned int			default_ovp;	//set overlay layer
	overlay_video_buffer_t	lastinfo;		//backup last_overlay_info
	
	pthread_mutex_t 		mutex_lock;
} DecodeDate;

static DecodeDate decode_data;


int tcc_vdec_init()
{
	//Init decode_data value
	decode_data.OverlayDrv = -1;
	decode_data.IsViewValid = 1;
	decode_data.IsDecoderOpen = 0;
	decode_data.IsConfigured = 0;
	decode_data.default_ovp = 24;
	
	pthread_mutex_init(&(decode_data.mutex_lock),NULL);
	
	return 0;
}

int tcc_vdec_open(void)
{	
	pthread_mutex_lock(&(decode_data.mutex_lock));
	memset( &(decode_data.lastinfo), 0, sizeof(overlay_video_buffer_t));
	
	//--------init and open vpu------------
	if(decode_data.IsDecoderOpen) {
		ErrorPrint( "Decoder is in use! It will be close!\n" );
		tcc_vpudec_close();
		decode_data.IsDecoderOpen = 0;
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
	if(iret < 0)
	{
		ErrorPrint("Annexb Header Decode Error!\n");
		pthread_mutex_unlock(&(decode_data.mutex_lock));
		return -1;
	}
	
	pthread_mutex_unlock(&(decode_data.mutex_lock));
	
	return 0;
}

#define LCD_WIDTH 1024
#define LCD_HEIGHT 600
#define MAX(a,b) (a>b)?a:b
#define TARGET_WIDTH 800.00
#define TARGET_HEIGHT 480.00
float TARGET_RATIO = (TARGET_WIDTH/TARGET_HEIGHT);

int tcc_vdec_process( unsigned char* data, int size)
{
	int iret = 0;
	unsigned int inputdata[4] = {0};
	unsigned int outputdata[15] = {0};
	unsigned int screen_width, screen_height;
	overlay_video_buffer_t info;
	
	pthread_mutex_lock(&(decode_data.mutex_lock));
	
	inputdata[0] = (unsigned int)data;
	inputdata[1] = (unsigned int)size;
	
	screen_width  = LCD_WIDTH;   //FIXME ; These value need to get from system.
	screen_height = LCD_HEIGHT;
	
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
	//////////////  Start to Push the video /////////////////////

	info.cfg.width = outputdata[8];
	info.cfg.height = outputdata[9];
	info.cfg.format = DEC_VIDEO_FORMAT;
	info.cfg.transform = 0;		
	info.addr = outputdata[1];		// Y Address;

#if !defined(CONFIG_OVERLAY_CROP) && !defined(CONFIG_OVERLAY_SCALE)
	//Set position X
	if(info.cfg.width <= screen_width)
	{
		info.cfg.sx =(screen_width-info.cfg.width)/2;
	}
	else
	{
		ErrorPrint("Image width is more then screen_width, need scaler!\n");
		info.cfg.sx = 0;
		info.cfg.width = screen_width;
	}
	//Set position Y
	if(info.cfg.height <= screen_height)
	{
		info.cfg.sy =(screen_height-info.cfg.height)/2;
	}
	else
	{
		ErrorPrint("Image height is more then screen_hieght, need scaler!\n");
		info.cfg.sy = 0;
		info.cfg.height = screen_height;
	}
#endif			
											
#if defined(CONFIG_OVERLAY_CROP)	
	info.addr1 = outputdata[2];		// U Address;
	info.addr2 = outputdata[3];		// V Address;
	
	//for crop
	info.cfg.crop_src.left = 0;
	info.cfg.crop_src.top = 0;
	info.cfg.crop_src.width = outputdata[8]-outputdata[13];
	info.cfg.crop_src.height = outputdata[9]-outputdata[14];
	
	info.cfg.sx = (LCD_WIDTH-info.cfg.crop_src.width)/2;
	info.cfg.sy = (LCD_HEIGHT-info.cfg.crop_src.height)/2;
	
	//Set position X
	if(info.cfg.crop_src.width <= screen_width)
	{
		info.cfg.sx =(screen_width-info.cfg.crop_src.width)/2;
	}
	else
	{
		ErrorPrint("Image width is more then screen_width, need scaler!\n");
		info.cfg.sx = 0;
		info.cfg.crop_src.width = screen_width;
	}
	//Set position Y
	if(info.cfg.crop_src.height <= screen_height)
	{
		info.cfg.sy =(screen_height-info.cfg.crop_src.height)/2;
	}
	else
	{
		ErrorPrint("Image height is more then screen_hieght, need scaler!\n");
		info.cfg.sy = 0;
		info.cfg.crop_src.height = screen_height;
	}
	DebugPrint("Pos [%d,%d], (%d,%d) -> (%d x %d)... \n", info.cfg.sx, info.cfg.sy, 
											info.cfg.width, info.cfg.height, 
											info.cfg.crop_src.width, info.cfg.crop_src.height);
#endif

	if(decode_data.IsViewValid)
	{	
		ioctl( decode_data.OverlayDrv, OVERLAY_PUSH_VIDEO_BUFFER, &info );
	}else{
		DebugPrint("View is invalid!\n");
	}
	memcpy( &(decode_data.lastinfo), &info, sizeof(overlay_video_buffer_t) );


	pthread_mutex_unlock(&(decode_data.mutex_lock));

	return 0;
}

