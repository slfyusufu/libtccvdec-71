//********************************************************************************************
/**
 * @file		libH264Decoder.c
 * @brief		要約説明
 *
 *	このファイルの詳細説明
 *
 * @author		N.Tanaka
 * @date		2014/10/14(火) 16:22:10
 */
//********************************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
//#define CONFIG_TCC_INVITE 1
#define CONFIG_ARCH_TCC897X 1
#include <tcc_overlay_ioctl.h>
#include <mach/vioc_global.h>

#include <string.h>
#include <pthread.h>

// Overlay Driver
#define	OVERLAY_DRIVER	"/dev/overlay"
static int g_OverlayDrv = -1;

//add for ovp setting
#define TCC_LCDC_SET_WMIXER_OVP         0x0045
#define FB_DEV "/dev/fb0"

// Decoder State
static int g_DecoderState = -1;

// >> For Debug
#define	DEBUG_MODE
#ifdef	DEBUG_MODE

	#define	DebugPrint( fmt, ... )	printf( "libH264Decoder(D):"fmt, ##__VA_ARGS__ )
	#define	ErrorPrint( fmt, ... )	printf( "libH264Decoder(E):"fmt, ##__VA_ARGS__ )

#else

	#define	DebugPrint( fmt, ... )
	#define	ErrorPrint( fmt, ... )	printf( "libH264Decoder(E):"fmt, ##__VA_ARGS__ )

#endif
// << For Debug

// 描画可否フラグ
static int g_IsViewValid = 0;	// 0:不可, 1:可

// SetConfigureを行ったかどうか
static int g_IsSetConfigure = 0;	// 0:未設定, 1:設定済
static unsigned int ignore = 1;
// 一番最後にDecodeした画像情報
static overlay_video_buffer_t g_lastinfo;

// Mutex
static pthread_mutex_t g_Mutex = PTHREAD_MUTEX_INITIALIZER;

static void SetConfigure(void)
{
	overlay_config_t cfg;
	unsigned int format = (unsigned int)'N' | (unsigned int)'V'<<8 | (unsigned int)'1'<<16 | (unsigned int)'2'<<24;
	cfg.sx = 0;
	cfg.sy = 0;
	cfg.width = 800;
	cfg.height = 480;
	cfg.format = format;
	cfg.transform = 0;
	ioctl( g_OverlayDrv, OVERLAY_SET_CONFIGURE, &cfg );
	g_IsSetConfigure = 1;
	
}

// 2015.04.23 N.Tanaka
// 描画可否のフラグを追加/設定する
int tcc_SetViewValidFlag(int isValid)
{
	pthread_mutex_lock(&g_Mutex);
	
	g_IsViewValid = isValid;
	
	// 無効から有効に状態が変わった際に、最後のデータを描画する
	if( isValid == 1 ){
		
		// Overlay Driverが開かれている時
		if( g_OverlayDrv >= 0 ){
			
			// SetConfigureを呼んでいない場合には呼ぶ
			if( g_IsSetConfigure == 0 ){
				
				SetConfigure();
			}
			
			// 最後のデコードデータが存在していればそれをDriverに渡す
			if( g_lastinfo.addr != 0 ){	// Addressが0かどうかでデータが存在しているのかを判断する
				ioctl( g_OverlayDrv, OVERLAY_PUSH_VIDEO_BUFFER, &g_lastinfo );
			}
		}
	}
	
	pthread_mutex_unlock(&g_Mutex);
	
	return 0;
}

void tcc_vdec_init(int sx, int sy, int width, int height)
{
	int visible=1;
	tcc_SetViewValidFlag(visible);

}
int tcc_vdec_open(void)
{
	overlay_config_t cfg;
	unsigned int format = VIOC_IMG_FMT_YUV420IL0;//(unsigned int)'N' | (unsigned int)'V'<<8 | (unsigned int)'1'<<16 | (unsigned int)'2'<<24;
	
	
	pthread_mutex_lock(&g_Mutex);
	
	// 2015.04.23 N.Tanaka SetConfigureしたかのフラグ。停止時にフラグ解除はしているが、念のためここでもフラグ解除
	g_IsSetConfigure = 0;
	
	memset( &g_lastinfo, 0, sizeof(overlay_video_buffer_t) );
	
	// AndroidAutoでCloseされないので、Openされている時には一度閉じてあげる
	if( g_DecoderState >= 0 ){
		ErrorPrint( "decoder is not closed... so stop decoder!!\n" );
		h264decoder_close_vdec();
		g_DecoderState = -1;
	}
	
	// Decoder準備 : 成功すると0, 失敗で-1が返ってくる
	if( g_DecoderState == -1 ){
		#if 0
		g_DecoderState = h264decoder_init_vdec(800, 480);
		#else
		// 2015.3.2 yuichi mod
		g_DecoderState = h264decoder_init_vdec(800, 476);
		#endif
	}
	
	// Overlay Driver準備
	if( g_OverlayDrv >= 0 ){
		close( g_OverlayDrv );
	}
	g_OverlayDrv = -1;
	
	g_OverlayDrv = open( OVERLAY_DRIVER, O_RDWR );
	if( g_OverlayDrv < 0 ){
		ErrorPrint( "Error : Overlay Driver Open Fail\n" );
	}else{
		cfg.sx = 0;
		cfg.sy = 0;
		cfg.width = 800;
		#if 0
		cfg.height = 480;
		#else
		// 2015.3.2 yuichi mod
		cfg.height = 480;
		#endif
		cfg.format = format;
		cfg.transform = 0;
		
		cfg.crop_src.left =0;
		cfg.crop_src.top =0;
		cfg.crop_src.width =cfg.width;
		cfg.crop_src.height =cfg.height;
		
		if(g_IsViewValid){	// 2015.04.23 : N.Tanaka 描画可否を判断する
//FINAL			ioctl( g_OverlayDrv, OVERLAY_SET_IGNORE_PRIORITY, &ignore );
			ioctl( g_OverlayDrv, OVERLAY_SET_CONFIGURE, &cfg );
			g_IsSetConfigure = 1;
		}
	}
#ifdef FINAL
	int fbdev;
	int ovp=8;
	fbdev = open(FB_DEV, O_RDWR);
	if ( fbdev < 0 ) {
		printf("Error opening %s.\n", FB_DEV);
		return 1;
	}
	if ( ioctl(fbdev, TCC_LCDC_SET_WMIXER_OVP, &ovp) < 0 ) {
		printf("FB Driver IOCTL ERROR\n");
		close(fbdev);
		return 1;
	}
#endif
	pthread_mutex_unlock(&g_Mutex);
	
	return 0;
}

int tcc_vdec_close(void)
{
	pthread_mutex_lock(&g_Mutex);
	
	if( g_OverlayDrv >= 0 ){
		close( g_OverlayDrv );
	}
	g_OverlayDrv = -1;
	g_IsSetConfigure = 0;	// 2015.04.23 : N.Tanaka
	
	if( g_DecoderState >= 0 ){
		h264decoder_close_vdec();
	}
	g_DecoderState = -1;
	
	memset( &g_lastinfo, 0, sizeof(overlay_video_buffer_t) );	// 2015.04.24 N.Tanaka
#ifdef FINAL	
	int fbdev;
	int ovp=24;
	fbdev = open(FB_DEV, O_RDWR);
	if ( fbdev < 0 ) {
		printf("Error opening %s.\n", FB_DEV);
		return 1;
	}
	if ( ioctl(fbdev, TCC_LCDC_SET_WMIXER_OVP, &ovp) < 0 ) {
		printf("FB Driver IOCTL ERROR\n");
		close(fbdev);
		return 1;
	}
#endif
	pthread_mutex_unlock(&g_Mutex);
	
	return 0;
}

int tcc_vdec_process_annexb_header( unsigned char* data, int datalen)
{
	int iret = 0;
	unsigned int outputdata[11] = {0};
	
	unsigned int inputdata[4] = {0};
	inputdata[0] = (unsigned int)data;
	inputdata[1] = (unsigned int)datalen;
	
	pthread_mutex_lock(&g_Mutex);
	
	if( g_DecoderState == -1 ){
		ErrorPrint( "decoder is not opened...\n" );
		pthread_mutex_unlock(&g_Mutex);
		return -1;
	}
	
	//iret = decoder_decode( data, datalen, outputdata );
	iret = h264decoder_decode(inputdata, outputdata);
	
	// Annex-Bヘッダは動画データではないので、描画要求はしない
	
	
	pthread_mutex_unlock(&g_Mutex);
	
	return 0;
}

#define MAX(a,b) (a>b)?a:b
#define TARGET_WIDTH 800.00
#define TARGET_HEIGHT 480.00
float TARGET_RATIO = (TARGET_WIDTH/TARGET_HEIGHT);
int tcc_vdec_process( unsigned char* data, int size)
{
	int iret = 0;
	unsigned int inputdata[4] = {0};
	unsigned int outputdata[15] = {0};
	overlay_video_buffer_t info;
	unsigned int crop_info[4]={0};
	unsigned int scaler_info[2]={0};
	
	pthread_mutex_lock(&g_Mutex);
	
	inputdata[0] = (unsigned int)data;
	inputdata[1] = (unsigned int)size;
	
	if( g_DecoderState == -1 ){
		ErrorPrint( "decoder is not opened...\n" );
		pthread_mutex_unlock(&g_Mutex);
		return -1;
	}
	
	//iret = decoder_decode( data, size, outputdata );
	iret = h264decoder_decode(inputdata, outputdata);
	
	if( iret >= 0 ){
		
		if( g_OverlayDrv >= 0 ){
			
//#ifdef FINAL
			info.cfg.width = outputdata[8];
			info.cfg.height = outputdata[9];
			info.cfg.format = VIOC_IMG_FMT_YUV420IL0;//(unsigned int)'N' | (unsigned int)'V'<<8 | (unsigned int)'1'<<16 | (unsigned int)'2'<<24;
			//info.cfg.format = 0;		// 使われてないようなので無視
			info.cfg.transform = 0;		// 使われてないようなので無視
			
			info.cfg.sx =0;
			info.cfg.sx =0;
			info.addr = outputdata[1];		// Y Address;
			#if 0	// 2015.3.2 yuichi add, UとVのアドレスも使う		
			info.addr1 = outputdata[2];
			info.addr2 = outputdata[3];
			#endif
			//for crop
			info.cfg.crop_src.left = 0;
			info.cfg.crop_src.top = 0;
			info.cfg.crop_src.width = outputdata[8]-outputdata[13];
			info.cfg.crop_src.height = outputdata[9]-outputdata[14];
printf("[libH264] (%d,%d) -> (%d x %d)... \n",info.cfg.width, info.cfg.height, \
											info.cfg.crop_src.width, info.cfg.crop_src.height);
#if 0			
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
			printf("[libH264] crop_width=%d, crop_height=%d\n",crop_info[2],crop_info[3]);
			printf("[libH264] Scaler: src (%d x %d) -- dst (%d x %d) \n", info.cfg.width, info.cfg.height, scaler_info[0], scaler_info[1]);
			printf("[libH264] (%d,%d) - (%d x %d)... \n",info.cfg.sx, info.cfg.sy, scaler_info[0], scaler_info[1]);

			ioctl( g_OverlayDrv, OVERLAY_SET_CROP_INFO, &crop_info);
			ioctl( g_OverlayDrv, OVERLAY_SET_SCALER_INFO, &scaler_info);
#endif			
			//printf( "%s: [0]=0x%08x, [1]=0x%08x, [2]=0x%08x\n", __func__, 
			//		info.addr, info.addr1, info.addr2 );	// yuichi
			
			if(g_IsViewValid){	// 2015.04.23 : N.Tanaka 描画可否を判断する
				
				// Start時にフラグが立っておらずSetConfigureされていない場合にはここでSetConfiguresする
				if( g_IsSetConfigure == 0 ){
					SetConfigure();
				}
				ioctl( g_OverlayDrv, OVERLAY_SET_CONFIGURE, &info.cfg );	
				ioctl( g_OverlayDrv, OVERLAY_PUSH_VIDEO_BUFFER, &info );
			}else{
				//printf("IsViewValid is false...\n");
			}
//#endif				
			// 2015.04.24 N.Tanaka
			// 最後のDecodeデータ情報を保持しておく
			memcpy( &g_lastinfo, &info, sizeof(overlay_video_buffer_t) );
			
		}else{
			ErrorPrint( "Decode but Overlay Driver is not opened\n" );
		}
		
	}else{
		
		ErrorPrint( "Decode fail\n" );
		
	}
	
	pthread_mutex_unlock(&g_Mutex);
	
	return 0;
}

