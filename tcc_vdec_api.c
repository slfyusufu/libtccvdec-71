//********************************************************************************************
/**
 * @file        tcc_vdec_api.h
 * @brief		Decode H264 frame and display image onto screen through overlay driver. 
 * 				This interface contain : 
 *
 * @author      Yusuf.Sha, Telechips Shenzhen Rep.
 * @date        2016/11/08
 */
//********************************************************************************************

#include "tcc_vdec_api.h"

#define	DEBUG_MODE
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

//static int g_OverlayDrv = -1;
// Decoder State
//static int g_DecoderState = -1;

//add for ovp setting
//static unsigned int default_ovp;

//static int g_IsViewValid = 0;
//static int g_IsSetConfigure = 0;	
//static overlay_video_buffer_t g_lastinfo;

// Mutex
//static pthread_mutex_t g_Mutex = PTHREAD_MUTEX_INITIALIZER;


typedef struct _DecodeDate {
	
	int 					OverlayDrv;		//overlay driver handler
	bool					IsViewValid;	//view valid ?
	bool					IsDecoderOpen;	//whether decode is in use ?
	bool					IsConfigured;	//whether be configured ?
	unsigned int			default_ovp;	//set overlay layer
	overlay_video_buffer_t	lastinfo;		//backup last_overlay_info
	
	pthread_mutex_t mutex_lock;
} DecodeDate;

static DecodeDate decode_data;

static void SetConfigure(void)
{
	overlay_config_t cfg;
	//unsigned int format = DEC_VIDEO_FORMAT;
	cfg.sx = 0;
	cfg.sy = 0;
	cfg.width = 800;
	cfg.height = 480;
	cfg.format = DEC_VIDEO_FORMAT;
	cfg.transform = 0;
	ioctl( decode_data.OverlayDrv, OVERLAY_SET_CONFIGURE, &cfg );
	decode_data.IsConfigured = 1;
	
}
#if 0
int tcc_SetViewValidFlag(int isValid)
{
	pthread_mutex_lock(&(decode_data.mutex_lock));
	
	g_IsViewValid = isValid;
	
	if( isValid == 1 ){
		if( decode_data.OverlayDrv >= 0 ){
			if( g_IsSetConfigure == 0 ){
				SetConfigure();
			}
			if( g_lastinfo.addr != 0 ){	
				ioctl( decode_data.OverlayDrv, OVERLAY_PUSH_VIDEO_BUFFER, &g_lastinfo );
			}
		}
	}
	
	pthread_mutex_unlock(&(decode_data.mutex_lock));
	
	return 0;
}
#endif
int tcc_vdec_init(int sx, int sy, int width, int height)
{
	//int visible=1;
	//tcc_SetViewValidFlag(visible);
	
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
	//overlay_config_t cfg;
	//unsigned int format = DEC_VIDEO_FORMAT;	
	
	pthread_mutex_lock(&(decode_data.mutex_lock));
	memset( &(decode_data.lastinfo), 0, sizeof(overlay_video_buffer_t));
	
	//--------init and open vpu------------
	if(decode_data.IsDecoderOpen) {
		ErrorPrint( "Decoder is in use! It will be close!\n" );
		tcc_vpudec_close();
		decode_data.IsDecoderOpen = 0;
	}
	if(tcc_vpudec_init(800, 476) < 0)
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
#if 0
		cfg.sx = 0;
		cfg.sy = 0;
		cfg.width = 800;
		cfg.height = 480;
		cfg.format = DEC_VIDEO_FORMAT;
		cfg.transform = 0;
#if defined(CONFIG_OVERLAY_CROP)		
		cfg.crop_src.left =0;
		cfg.crop_src.top =0;
		cfg.crop_src.width =cfg.width;
		cfg.crop_src.height =cfg.height;
#endif
#if 0	
		if(g_IsViewValid){	
			ioctl( g_OverlayDrv, OVERLAY_SET_CONFIGURE, &cfg );
			g_IsSetConfigure = 1;
		}
#endif
#endif
		//set overlay to the top layer, otherwise, we overlay maybe covered by UI.
		//This value need set accroding to layer which use in overlay driver
		unsigned int overlay_ovp=0; 
		ioctl(decode_data.OverlayDrv, OVERLAY_GET_WMIXER_OVP, &decode_data.default_ovp); //backup default_ovp
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
		//pthread_mutex_unlock(&(decode_data.mutex_lock));
		//return -1;
	}		
	//////////////  Start to Push the video /////////////////////
	
			info.cfg.width = outputdata[8];
			info.cfg.height = outputdata[9];
			info.cfg.format = DEC_VIDEO_FORMAT;
			info.cfg.transform = 0;		
			info.addr = outputdata[1];		// Y Address;
			
			//DebugPrint("Video format is 0x%x\n",info.cfg.format);
			
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
#if defined(CONFIG_OVERLAY_SCALE)			
			///for scaler
			float ratio0 = (float)info.cfg.width/(float)info.cfg.height;
			float ratio1 = (float)info.cfg.height/(float)info.cfg.width;
			//printf("[Yusuf] ratio0=%.5f, ratio1=%.5f, target_ratio=%.5f...\n",ratio0, ratio1, TARGET_RATIO);
			if((ratio0 >= TARGET_RATIO) || (ratio1 >= TARGET_RATIO)) //ratio is 16:9
			{
				if(info.cfg.width > info.cfg.height)
				{
					scaler_info[0] = TARGET_WIDTH;
					//scaler_info[1] = (800*9)/16;
					scaler_info[1] = (TARGET_WIDTH*(float)crop_info[3])/(float)crop_info[2];
				}else{
					//scaler_info[0] = (480*9)/16;
					scaler_info[0] = (TARGET_HEIGHT*(float)crop_info[2])/(float)crop_info[3];
					scaler_info[1] = TARGET_HEIGHT;
				}
			}else{//ratio is 4:3
				if(info.cfg.width > info.cfg.height)
				{
					scaler_info[0] = (480*4)/3;//(unsigned int)(480.00 * (MAX(ratio0,ratio1))); //640;
					scaler_info[1] = 480;
				}else{
					scaler_info[0] = (480*3)/4;//(unsigned int)(480.00 / (MAX(ratio0,ratio1))); //360;
					scaler_info[1] = 480;
				}
			}
			info.cfg.sx = (800-scaler_info[0])/2;
			info.cfg.sy = (480-scaler_info[1])/2;
			printf("[libH264] Scaler: src (%d x %d) -- dst (%d x %d) \n", info.cfg.width, info.cfg.height, scaler_info[0], scaler_info[1]);
			printf("[libH264] (%d,%d) - (%d x %d)... \n",info.cfg.sx, info.cfg.sy, scaler_info[0], scaler_info[1]);

			//ioctl( g_OverlayDrv, OVERLAY_SET_CROP_INFO, &crop_info);
			//ioctl( g_OverlayDrv, OVERLAY_SET_SCALER_INFO, &scaler_info);
#endif			
			
			//if(g_IsViewValid)
			{	
				
				if( decode_data.IsConfigured == 0 ){
					SetConfigure();
				}
				ioctl( decode_data.OverlayDrv, OVERLAY_SET_CONFIGURE, &info.cfg );	
				ioctl( decode_data.OverlayDrv, OVERLAY_PUSH_VIDEO_BUFFER, &info );
			//}else{
				//printf("IsViewValid is false...\n");
			}

			memcpy( &(decode_data.lastinfo), &info, sizeof(overlay_video_buffer_t) );
			
	
	pthread_mutex_unlock(&(decode_data.mutex_lock));
	
	return 0;
}

