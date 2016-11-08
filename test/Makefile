#
# Makefile
#
# Makefile for tccvdec test


CC	= /opt/arm-2013.11/bin/arm-none-linux-gnueabi-gcc 
#CC = gcc
CFLAGS  = -O2 -Wall -D_GNU_SOURCE 
#-static -lpthread -ldl

SOURCE_PATH=/home/s100018/mywork/linux/telechips/als-v1.0-r03/build/tcc8971-lcn_v1.0/tmp/work/cortexa7-vfp-neon-telechips-linux-gnueabi

CFLAGS += -I./..
LDLIBS += -L./../lib -ltccvdec
CFLAGS += -I$(SOURCE_PATH)/libomxil-telechips/1.0.0-r0/git/src/omx/omx_videodec_interface/include/
LDLIBS += -L$(SOURCE_PATH)/libomxil-telechips/1.0.0-r0/image/usr/lib -lomxvideodec -lomxbase -lomxvideodeccore
CFLAGS += -I$(SOURCE_PATH)/glib-2.0/1_2.44.1-r0/image/usr/include
CFLAGS += -I$(SOURCE_PATH)/glib-2.0/1_2.44.1-r0/image/usr/lib/glib-2.0/include
LDLIBS += -L$(SOURCE_PATH)/glib-2.0/1_2.44.1-r0/image/usr/lib -lgio-2.0 -lgobject-2.0 -lglib-2.0 -lgmodule-2.0 -lgthread-2.0
CFLAGS += -I$(SOURCE_PATH)/libffi/3.2.1-r0/image/usr/lib/libffi-3.2.1/include
LDLIBS += -L$(SOURCE_PATH)/libffi/3.2.1-r0/image/usr/lib -lffi
CFLAGS += -I$(SOURCE_PATH)/zlib/1.2.8-r0/image/usr/include
LDLIBS += -L$(SOURCE_PATH)/zlib/1.2.8-r0/image/usr/lib -lz

#LDLIBS =  -L/home/yusufu/mywork/workspace/linux/wav-player/alsa-lib/lib/ -lasound
#LDLIBS += -L/media/tcsz/mywork/JVC/linux-platform/myout/app_rom/tccgst/lib -lomxvideodec -lomxbase -lomxvideodeccore -lgio-2.0 -lgobject-2.0 -lffi -lglib-2.0 -lgmodule-2.0 -liconv
#LDLIBS += -L/media/tcsz/mywork/JVC/test/zlib_arm/lib -lz

SOURCES	= main.c
OBJECTS	= ${SOURCES:.c=.o}

OUT	= main

all: $(OUT)
	@echo Build DONE.

$(OUT): $(OBJECTS)
	$(CC)  -o $(OUT)  $(OBJECTS) $(CFLAGS)  $(LDLIBS)

clean:
	rm -f $(OBJECTS) *~ $$$$~* *.bak core config.log $(OUT)


##arm-none-linux-gnueabi-gcc -I/media/tcsz/mywork/JVC/SampleCodes/libH264Decoder  -L/media/tcsz/mywork/JVC/SampleCodes/libH264Decoder/lib -lh264decoder -L/media/tcsz/mywork/JVC/linux-platform/myout/app_rom/tccgst/lib -lomxvideodec -lomxbase -lomxvideodeccore -lgio-2.0 -lgobject-2.0 -lffi -lglib-2.0 -lgmodule-2.0 -liconv play.c -o play


##arm-none-linux-gnueabi-gcc -o h264 main.c -I./ -L./lib -lh264decoder -I/home/s100018/mywork/linux/telechips/als-linux-official/build/tcc8930/tmp/work/cortexa9-vfp-neon-telechips-linux-gnueabi/libomxil-telechips/1.0.0-r0/git/src/omx/omx_videodec_interface/include/ -L/home/s100018/mywork/linux/telechips/als-linux-official/build/tcc8930/tmp/work/cortexa9-vfp-neon-telechips-linux-gnueabi/libomxil-telechips/1.0.0-r0/image/usr/lib -lomxvideodec
