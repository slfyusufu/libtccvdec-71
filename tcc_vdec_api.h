//********************************************************************************************
/**
 * @file		libH264Decoder.c.h
 * @brief		要約説明
 *
 *	このファイルの詳細説明
 *
 * @author		N.Tanaka
 * @date		2014/11/04(火) 16:05:27
 */
//********************************************************************************************

#ifndef	__LIBH264DECODER_H__
#define	__LIBH264DECODER_H__

#include <stdint.h>	// for uint64_t

#ifdef	__cplusplus
extern "C"{
#endif

int tcc_vdec_open(void);
int tcc_vdec_close(void);
int tcc_vdec_process_annexb_header( unsigned char* data, int datalen);
int tcc_vdec_process( unsigned char* data, int size);
int libH264Decoder_SetViewValidFlag(int isValid);
int tcc_vdec_init(int x, int y, int w, int h);

#ifdef	__cplusplus
}
#endif

#endif	// __LIBH264DECODER_H__
