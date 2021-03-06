//********************************************************************************************
/**
 * @file        tcc_vpudec_intf.h
 * @brief		Decode one video frame using TCC VPU, now, we just set it to H264 decode. 
 * 				This interface contain : Init VPU, Decode frame and Close VPU.
 *
 * @author      Telechips Shenzhen Rep.
 * @date        2016/11/08
 */
//********************************************************************************************
#ifndef	__TCC_VPUDEC_INTF_H__
#define	__TCC_VPUDEC_INTF_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vdec_v2.h"

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

/***********************************************************/
//INSTANCE Stuct
#define TS_TIMESTAMP_CORRECTION
#define RESTORE_DECODE_ERR
#define CHECK_SEQHEADER_WITH_SYNCFRAME
/** The output decoded color format */
#define EXT_V_DECODER_TR_TEST


#ifdef TS_TIMESTAMP_CORRECTION
typedef struct ts_pts_ctrl{
	int m_iLatestPTS;
	int m_iPTSInterval;
	int m_iRamainingDuration;
} ts_pts_ctrl;
ts_pts_ctrl gsTSPtsInfo;
#endif

typedef struct _VIDEO_DECOD_INSTANCE_ {
	int avcodecReady;
	unsigned int video_coding_type;
	unsigned char	container_type;
	unsigned int  bitrate_mbps;
	vdec_input_t gsVDecInput;
	vdec_output_t gsVDecOutput;
	vdec_init_t gsVDecInit;
	vdec_user_info_t gsVDecUserInfo;
#ifdef TIMESTAMP_CORRECTION
	pts_ctrl gsPtsInfo;
#endif
	int  isVPUClosed;
	unsigned int video_dec_idx;
//	cdmx_info_t cdmx_info;	
	dec_disp_info_ctrl_t dec_disp_info_ctrl;
	dec_disp_info_t dec_disp_info[32];
	dec_disp_info_input_t dec_disp_info_input;
	void* pVdec_Instance;
	ts_pts_ctrl gsTSPtsInfo;
	cdk_func_t *gspfVDec;
	unsigned int 	restred_count;
#ifdef EXT_V_DECODER_TR_TEST
	int gsextTRDelta;
	int gsextP_frame_cnt;
	int gsextReference_Flag;
	EXT_F_frame_time_t gsEXT_F_frame_time;
#endif
} _VIDEO_DECOD_INSTANCE_;

/***********************************************************/
//INTERNAL VARIABLE
typedef struct dec_private_data {
//dec operation
//	vdec_init_t 		gsVDecInit;
//	vdec_input_t 		gsVDecInput;
//	vdec_output_t 		gsVDecOutput;
//	vdec_user_info_t 	gsVDecUserInfo;
	unsigned char 		isSequenceHeaderDone;
//	unsigned char 		isVPUClosed;

//info
//	unsigned char		container_type;
//	unsigned int 		video_coding_type;
	unsigned int 		frameSearchOrSkip_flag;
	unsigned char 		isFirst_Frame;
	unsigned char  		i_skip_scheme_level;
	signed char  		i_skip_count;
	signed char  		i_skip_interval;
	unsigned char 		bUseFrameDefragmentation;
	unsigned char		nFps;

	_VIDEO_DECOD_INSTANCE_ pVideoDecodInstance;
	mpeg2_pts_ctrl gsMPEG2PtsInfo;
	unsigned int out_index;
	unsigned int in_index;
	unsigned int frm_clear;
	unsigned int Display_index[VPU_BUFF_COUNT];
	unsigned int max_fifo_cnt;
//error process
	signed int 		seq_header_init_error_count;
	unsigned int 		ConsecutiveVdecFailCnt;
	signed int			ConsecutiveBufferFullCnt;

#ifdef RESTORE_DECODE_ERR
	unsigned char* 		seqHeader_backup;
	unsigned int 		seqHeader_len;
	unsigned int 		cntDecError;
#endif

#ifdef CHECK_SEQHEADER_WITH_SYNCFRAME
	unsigned char*		sequence_header_only;
	long 		sequence_header_size;
	unsigned char 		need_sequence_header_attachment;
#endif
}tDEC_PRIVATE;

int  tcc_vpudec_init( int width, int height );
int  tcc_vpudec_decode(unsigned int *pInputStream, unsigned int *pOutstream);
void tcc_vpudec_close(void);

#endif	// __TCC_VPUDEC_INTF_H__
