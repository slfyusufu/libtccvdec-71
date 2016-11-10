//********************************************************************************************
/**
 * @file        main.c
 * @brief		Test demo for libtccvdec. 
 *
 * @author      Yusuf.Sha, Telechips Shenzhen Rep.
 * @date        2016/11/08
 */
//********************************************************************************************

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <tcc_vdec_api.h>
static int h264RawDataDecode()
{
	int FileNo = 0;
	FILE* fp;
	const char filepath_base[] = "/home/root/dump/";
	char filepath[260] = {0};
	char* data = NULL;
	int g_IsAnnexB_OtherFile = 1;
	
	if( g_IsAnnexB_OtherFile == 1 ){	// Annex-B
		FILE* annexb = fopen( "/home/root/dump/header.264", "rb" );
		if( !annexb ){
			printf( "Error!! ANNEX-B Header File Can't Open\n" );
		}else{
			int iAnnex = fseek( annexb, 0, SEEK_END );
			if( iAnnex < 0 ){
				printf( "Error Fseek\n" );
			}else{
				long AnnexLen = ftell( annexb );
				if( AnnexLen <= 0 ){
					printf( "Annex File Size Error [AnnexLen:%d]\n", (int)AnnexLen );
				}else{
					fseek( annexb, 0, SEEK_SET );
					char* AnnexBuf = (char*)malloc( AnnexLen );
					if( !AnnexBuf ){
						printf( "Error Annex malloc Fail\n" );
					}else{
						int AnnexRead = fread( AnnexBuf, 1, AnnexLen, annexb );
						if( AnnexRead < AnnexLen ){
							printf( "Annex File Read Fail?\n" );
						}
						// Decode
						tcc_vdec_process_annexb_header( (unsigned char*)AnnexBuf, AnnexLen);
						printf( "Decoded Annex-B Header!\n" );
					}
				}
			}
		}
		
	}

	while( 1 )
	{
		memset( filepath, 0, sizeof(filepath) );
		sprintf( filepath, "%s%04d.h264", filepath_base, FileNo );
		FileNo++;
		
		// File Open
		fp = fopen(filepath,"r" );
		if( !fp ){
			printf( "%s can't open\n", filepath );
			break;
		}
		int iret;
		long lSize;
#if 1		
		// SizeCheck
		iret = fseek( fp, 0, SEEK_END );
		if( iret < 0 ){
			printf( "fseek fail\n" );
			fclose(fp);
			break;
		}
		lSize = ftell( fp );
		if( lSize < 0 ){
			printf( "ftell fail\n" );
			fclose(fp);
			break;
		}
		iret = fseek( fp, 0, SEEK_SET );
		if( iret < 0 ){
			printf( "fseek - 2 fail\n" );
			fclose(fp);
			break;
		}
#endif		
		// File Read
		data = (char*)malloc( lSize );
		if( data == NULL ){
			printf( "malloc error\n" );
			fclose(fp);
			break;
		}
		fread( data, 1, lSize, fp );
		
		// File Close
		fclose( fp );
		
		// Decode
		iret = tcc_vdec_process( (unsigned char*)data, (int)lSize);
		
		if( iret < 0 ){
			//printf( "Decode Error [%d]\n", (FileNo-1) );
		}
		
		free( data );
		data = NULL;
		
		usleep( 10000 );
	}
	
	return 0;
}

int main( int argc, char** argv )
{
	tcc_vdec_init();
	tcc_vdec_open();
	
	h264RawDataDecode();
	
	tcc_vdec_close();
	
	return 0;
}

