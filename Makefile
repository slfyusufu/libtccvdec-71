PATH_ARM_NONE_LINUX_GNUEABI := /opt/arm-2013.11
COMPILER = $(PATH_ARM_NONE_LINUX_GNUEABI)/bin/arm-none-linux-gnueabi-gcc
STRIP = $(PATH_ARM_NONE_LINUX_GNUEABI)/bin/arm-none-linux-gnueabi-strip
CFLAGS   = -Wall -g -MMD -MP -mcpu=cortex-a7 -DHAVE_ANDROID_OS
LDFLAGS  =
LIBS     =
INCLUDE +=  -I$(PATH_ARM_NONE_LINUX_GNUEABI)/arm-none-linux-gnueabi/libc/usr/include
INCLUDE +=  -I$(PATH_ARM_NONE_LINUX_GNUEABI)/arm-none-linux-gnueabi/include

LDFLAGS += -L$(PATH_ARM_NONE_LINUX_GNUEABI)/arm-none-linux-gnueabi/libc/usr/lib
LDFLAGS += -L$(PATH_ARM_NONE_LINUX_GNUEABI)/arm-none-linux-gnueabi/libc/lib

OBJDIR = ./obj
TARGETDIR = ./lib
PLATFORM = tcc8971
SOURCE_PATH=/home/s100018/mywork/linux/telechips/als-v1.0-r03/build/tcc8971-lcn_v1.0/tmp/work
SHAREDLIB_FLAGS := -shared -fPIC

OBJECTS  = $(addprefix $(OBJDIR)/, $(SOURCES:.c=.o) )
DEPENDS  = $(OBJECTS:.o=.d)

CPP_OBJECTS  = $(addprefix $(OBJDIR)/, $(CPP_SOURCES:.cpp=.o) )
OBJECTS  += $(CPP_OBJECTS)
DEPENDS  += $(CPP_OBJECTS:.o=.d)


###//////////////for 8971
ifeq ($(PLATFORM), tcc8971)
CFLAGS += -I$(SOURCE_PATH)/cortexa7-vfp-neon-telechips-linux-gnueabi/libomxil-telechips/1.0.0-r0/git/src/omx/omx_videodec_interface/include
CFLAGS += -I$(SOURCE_PATH)/tcc8971_lcn-telechips-linux-gnueabi/linux-telechips/3.18.24-r0/git/arch/arm/mach-tcc897x/include/mach
CFLAGS += -I$(SOURCE_PATH)/tcc8971_lcn-telechips-linux-gnueabi/linux-telechips/3.18.24-r0/git/arch/arm/mach-tcc897x/include/
endif

# Telechips OMX
#LIB_DIR = /media/tcsz/mywork/JVC/linux-platform/myout/app_rom/tccgst/lib
#LIB_DIR = /home/s100018/mywork/JVC/airplay_kl/ExternalLibs/Middleware/lib
#LIB_DIR = /home/s100018/mywork/linux/telechips/als-linux-official/build/tcc8935-lcn/tmp/work/cortexa9-vfp-neon-telechips-linux-gnueabi/libomxil-telechips/1.0.0-r0/image/usr/lib
#LIB_DIR = /home/s100018/mywork/linux/telechips/als-linux-official/build/tcc8930/tmp/work/cortexa9-vfp-neon-telechips-linux-gnueabi/libomxil-telechips/1.0.0-r0/image/usr/lib
LIB_DIR = $(SOURCE_PATH)/cortexa7-vfp-neon-telechips-linux-gnueabi/libomxil-telechips/1.0.0-r0/image/usr/lib
LIB_FILES = -lomxvideodec
LDFLAGS += -L$(LIB_DIR)

# SharedLib Linker Option
LINK_PARAM = -Wl,-no-as-needed -Wl,-rpath-link $(LIB_DIR)

# Target Setting
TARGET = $(TARGETDIR)/libtccvdec.so
SOURCES  = tcc_vdec_api.c tcc_vpudec_intf.c
#CPP_SOURCES = libH264DecoderCpp.cpp

$(TARGET): $(OBJECTS) $(LIBS)
	@[ -d "./lib" ] || mkdir -p "./lib"
	$(COMPILER) $(SHAREDLIB_FLAGS) $(LINK_PARAM) $(LDFLAGS) $(LIB_FILES) -o $@ $^
#	$(STRIP) $(TARGET)

$(OBJDIR)/%.o: %.c
	@[ -d $(OBJDIR) ] || mkdir -p $(OBJDIR)
	$(COMPILER) -fPIC $(CFLAGS) $(INCLUDE) $(LDFLAGS) -o $@ -c $<

$(OBJDIR)/%.o: %.cpp
	@[ -d $(OBJDIR) ] || mkdir -p $(OBJDIR)
	$(COMPILER) -fPIC $(CFLAGS) $(INCLUDE) $(LDFLAGS) -o $@ -c $<

all: clean $(TARGET)

clean:
	rm -f $(OBJECTS) $(DEPENDS) $(TARGET)
	rm -rf $(OBJDIR)
	rm -rf $(TARGETDIR)

-include $(DEPENDS)

