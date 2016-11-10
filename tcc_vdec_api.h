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

#ifndef	__TCC_VDEC_API_H__
#define	__TCC_VDEC_API_H__

#include <stdint.h>	// for uint64_t

#ifdef	__cplusplus
extern "C"{
#endif

int tcc_vdec_open(void);
int tcc_vdec_close(void);
int tcc_vdec_process_annexb_header( unsigned char* data, int datalen);
int tcc_vdec_process( unsigned char* data, int size);
//int libH264Decoder_SetViewValidFlag(int isValid);
int tcc_vdec_init(int x, int y, int w, int h);

#ifdef	__cplusplus
}
#endif

#endif	// __TCC_VDEC_API_H__
