PATH_ARM_NONE_LINUX_GNUEABI := /opt/arm-2013.11
COMPILER = $(PATH_ARM_NONE_LINUX_GNUEABI)/bin/arm-none-linux-gnueabi-gcc
STRIP = $(PATH_ARM_NONE_LINUX_GNUEABI)/bin/arm-none-linux-gnueabi-strip
CFLAGS   = -Wall -g -MMD -MP
LDFLAGS  =
LIBS     =
INCLUDE +=  -I$(PATH_ARM_NONE_LINUX_GNUEABI)/arm-none-linux-gnueabi/libc/usr/include
INCLUDE +=  -I$(PATH_ARM_NONE_LINUX_GNUEABI)/arm-none-linux-gnueabi/include

LDFLAGS += -L$(PATH_ARM_NONE_LINUX_GNUEABI)/arm-none-linux-gnueabi/libc/usr/lib
LDFLAGS += -L$(PATH_ARM_NONE_LINUX_GNUEABI)/arm-none-linux-gnueabi/libc/lib

OBJDIR = ./obj
TARGETDIR = ./lib
PLATFORM = tcc8971
SOURCE_PATH=/home/s100018/mywork/linux/telechips/als-v1.0-r03/build/tcc8971-lcn_v1.0/tmp/work/cortexa7-vfp-neon-telechips-linux-gnueabi
KERNEL_PATH=/home/s100018/mywork/linux/telechips/als-v1.0-r03/build/tcc8971-lcn_v1.0/tmp/work-shared/tcc8971-lcn-2.0/kernel-source
SHAREDLIB_FLAGS := -shared -fPIC

OBJECTS  = $(addprefix $(OBJDIR)/, $(SOURCES:.c=.o) )
DEPENDS  = $(OBJECTS:.o=.d)

CPP_OBJECTS  = $(addprefix $(OBJDIR)/, $(CPP_SOURCES:.cpp=.o) )
OBJECTS  += $(CPP_OBJECTS)
DEPENDS  += $(CPP_OBJECTS:.o=.d)


###//////////////for 8971
ifeq ($(PLATFORM), tcc8971)

CFLAGS += -mcpu=cortex-a7 -DHAVE_ANDROID_OS
CFLAGS += -I$(SOURCE_PATH)/libomxil-telechips/1.0.0-r0/git/src/omx/omx_videodec_interface/include
CFLAGS += -I$(KERNEL_PATH)/arch/arm/mach-tcc897x/include/

endif

LIB_DIR = $(SOURCE_PATH)/libomxil-telechips/1.0.0-r0/image/usr/lib
LIB_FILES = -lomxvideodec
LDFLAGS += -L$(LIB_DIR)

# SharedLib Linker Option
LINK_PARAM = -Wl,-no-as-needed -Wl,-rpath-link $(LIB_DIR)

# Target Setting
TARGET = $(TARGETDIR)/libtccvdec.so
SOURCES  = tcc_vdec_api.c tcc_vpudec_intf.c

$(TARGET): $(OBJECTS) $(LIBS)
	@[ -d "./lib" ] || mkdir -p "./lib"
	$(COMPILER) $(SHAREDLIB_FLAGS) $(LINK_PARAM) $(LDFLAGS) $(LIB_FILES) -o $@ $^
	$(STRIP) $(TARGET)

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

