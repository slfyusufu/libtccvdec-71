#ifndef	__TCC_VPUDEC_INTF_H__
#define	__TCC_VPUDEC_INTF_H__

#include <stdlib.h>


/***********************************************************/
//COMMON PARAMETERS
#ifndef COMMON_CODEC_PARAM
#define COMMON_CODEC_PARAM
typedef enum CODEC_FORMAT {
	CODEC_FORMAT_H263 = 0,
	CODEC_FORMAT_MPEG4,
	CODEC_FORMAT_H264,	
	CODEC_FORMAT_RV,     	// Decoder only
	CODEC_FORMAT_MPEG2,     // Decoder only 
	CODEC_FORMAT_DIV3,      // Decoder only
	CODEC_FORMAT_VC1,       // Decoder only
	CODEC_FORMAT_MJPG,      // Decoder only
	CODEC_FORMAT_ILLEGAL
} tCODEC_FORMAT;

typedef enum pic_type {
	TYPE_I_FRAME	= 0,
	TYPE_P_FRAME,	
	TYPE_B_FRAME,	
	TYPE_ILLEGAL	
} tPIC_TYPE;
#endif


/***********************************************************/
//DECODER PARAMETERS
typedef enum CONTAINER_TYPE {
	CONTAINER_NONE = 0,
	CONTAINER_MKV,
	CONTAINER_MP4,
	CONTAINER_AVI,
	CONTAINER_MPG,
	CONTAINER_TS,
	CONTAINER_ASF,
	CONTAINER_RMFF,
	CONTAINER_FLV = 10
} tCONTAINER_TYPE;

typedef enum frame_buf_format {
	FRAME_BUF_FORMAT_YUV420P	= 0,	/*  	YUV420 planar : Y field + U field + V field*/
	FRAME_BUF_FORMAT_YUV420I,			/*  	YUV420 interleaved : Y field + UV field. */
	FRAME_BUF_FORMAT_ILLEGAL	
} tFRAME_BUF_FORMAT;

typedef struct dec_init_params {
	tCONTAINER_TYPE container_type; /* Muxer type */
	tCODEC_FORMAT 	codecFormat;   	/* Decoder's input format */
	int 			picWidth;		/* Decoder's picture width, by pixels */
	int				picHeight;		/* Decoder's picture height, by pixels */
} tDEC_INIT_PARAMS;

typedef struct dec_frame_input {
	unsigned char	*inputStreamAddr;	/* Base address of input bitstream (virtual address) */
	int				inputStreamSize;	/* length of input bitstream, by Bytes */
	unsigned char	seek;				/* in case of seek */
	int				nTimeStamp;			/* TimeStamp of input bitstream, by ms */
} tDEC_FRAME_INPUT;


typedef struct dec_frame_output {
	tFRAME_BUF_FORMAT	frameFormat;
	unsigned int		bufPhyAddr[3];		/* (Y,U,V or Y, UV). Base address of output frame (physical address) */
	unsigned int		bufVirtAddr[3];		/*  (Y,U,V or Y, UV). Base address of output frame (virtual address) */
	int					nTimeStamp;			/* TimeStamp of output bitstream, by ms */
	int					picWidth;			/* Picture width, by pixels */
	int					picHeight;			/* Picture height, by pixels */
	int					stride;
	//add by yusufu for crop info
	unsigned int 		crop_left;
	unsigned int 		crop_top;
	unsigned int 		crop_right;
	unsigned int 		crop_bottom;
} tDEC_FRAME_OUTPUT;

typedef struct dec_result {
	int 	need_input_retry;	/* same input stream has to put again because decoder is failed in case of internal buffer full. */
	int 	no_frame_output;	/* can't display because decoder fail or more frame need. */
} tDEC_RESULT;

int h264decoder_init_vdec( int width, int height );
void h264decoder_close_vdec(void);
int h264decoder_decode(unsigned int *pInputStream, unsigned int *pOutstream);

#endif	// __TCC_VPUDEC_INTF_H__
