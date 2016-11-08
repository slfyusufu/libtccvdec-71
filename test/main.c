
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <tcc_vdec_api.h>
static int h264RawDataDecode()
{
	int FileNo = 0;
	FILE* fp;
//	const char filepath_base[] = "/mnt/SD1p1/dump_data/data_";
	const char filepath_base[] = "/home/root/dump/";
//	const char filepath_base[] = "/nand1/dump_data/";
//	char *filepath_base = argv;
	char filepath[260] = {0};
	char* data = NULL;
	
	int g_IsAnnexB_OtherFile = 1;
	
	if( g_IsAnnexB_OtherFile == 1 ){	// Annex-Bヘッダを別ファイルとして保持している設定の場合
		
		//FILE* annexb = fopen( "/run/media/sda1/myself/h264/annexb/data_0000.h264", "r" );
		//FILE* annexb = fopen( "/run/media/sda1/myself/h264/test.264", "rb" );
		FILE* annexb = fopen( "/home/root/dump/header.264", "rb" );
		if( !annexb ){
			printf( "Error!! ANNEX-B Header File Can't Open\n" );
		}else{
			printf( "Annex-B Header Test\n" );
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
						printf( "Annex-B Header Test ..... end\n" );
					}
				}
			}
		}
		
	}
	//libH264Decoder_SetViewValidFlag(1);
	while( 1 )
	{
		
		memset( filepath, 0, sizeof(filepath) );
		sprintf( filepath, "%s%04d.h264", filepath_base, FileNo );
		FileNo++;
		
		// File Open
		fp = fopen(filepath,"r" );
//		fp = fopen("/home/root/test.264","rb" );
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
		
		// Unite File >>
		#if 0
		FILE* fw = fopen( "/temp/AAA.h264", "a" );
		if( !fw ){
			printf( "Unite File Open fail\n" );
		}else{
			fwrite( data, 1, lSize, fw );
			fclose( fw );
		}
		#endif
		// <<
		
		// File Close
		fclose( fp );
		
		// Decode
		unsigned int outputdata[11] = {0};
		iret = tcc_vdec_process( (unsigned char*)data, (int)lSize);
		
		if( iret < 0 ){
			printf( "Decode Error [%d]\n", (FileNo-1) );
		}
		
		free( data );
		data = NULL;
		
		usleep( 20000 );
	}
	
	return 0;
}

int main( int argc, char** argv )
{
	unsigned char* data = NULL;
	int size = 0;
	uint64_t inDisplayTick = 0;
	
	tcc_vdec_init(0,0,800,480);
	tcc_vdec_open();
	
//	libH264Decoder_AnnexBHeader_Write( data, size );
	
//	libH264Decoder_Write( data, size, inDisplayTick );
	
	h264RawDataDecode();
	
	tcc_vdec_close();
	
	return 0;
}

