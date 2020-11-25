#REV: requires FS, so probably g++-8 or better...
CXX=g++
CC=gcc

OCV_VERS=opencv4

AVCODEC_INCL:=`pkg-config --cflags libavcodec`
AVCODEC_LIBS:=`pkg-config --libs libavcodec`
#GSLINCL:=`pkg-config --cflags gsl`
#GSLSO:=`pkg-config --libs gsl`
#CVINCL=`pkg-config --cflags $(OCV_VERS)`
#CVSO=`pkg-config --libs $(OCV_VERS)`

#REV: just do both opencv and opencv4...
CVINCL:=`pkg-config --silence-errors --cflags opencv` `pkg-config --silence-errors --cflags opencv4`
CVSO:=`pkg-config --silence-errors --libs opencv` `pkg-config --silence-errors --libs opencv4`

BUILD_BASE ?= ./build

SRC_DIRS ?= ./cpp
INC_DIRS ?= ./include ./boost

LDFLAGS += $(CVSO) $(AVCODEC_LIBS)

SHARED = -fPIC -shared

INCLS := $(shell find $(INC_DIRS) -name *.hpp -or -name *.h )
SRCS := $(shell find $(SRC_DIRS) -name *.cpp )

INC_FLAGS := $(addprefix -I,$(INC_DIRS)) $(CVINCL) $(AVCODEC_INCL) -I.
CPPFLAGS ?= $(INC_FLAGS) -MMD -MP -std=c++17 -Wall -pthread -O2 -g #-Wshadow -Werror=shadow

################### LINUX amd64 shared #############################
ifeq ($(MAKECMDGOALS),librteye2.so)
TARGET_EXEC = $(MAKECMDGOALS)
CPPFLAGS += $(SHARED)
BUILD_DIR = $(BUILD_BASE)/WORKSPACE_$(MAKECMDGOALS)
endif


#################### IOS ARM 64 #############################
ifeq ($(MAKECMDGOALS),librteye2_ios.a)

TARGET_EXEC := $(MAKECMDGOALS)
BUILD_DIR := $(BUILD_BASE)/WORKSPACE_$(MAKECMDGOALS)

#REV: ###### CHANGE THIS TO REFLECT REAL LOCATION
APPLE_OCV_PATH := /Users/riveale/git/standout/opencv2.framework
APPLE_OCV_INCL := $(APPLE_OCV_PATH)/Headers

#REV: I actually don't have to link anything...these are static libs, and they will be linked at runtime...? I.e. in the swift code side heh.

#REV: #######  CHANGE THIS TO REFLECT REAL LOCATION
APPLE_FFMPEG_PATH := /Users/riveale/git/standout/mobile-ffmpeg-min-universal
APPLE_FFMPEG_INCL := $(APPLE_FFMPEG_PATH)/include

FULLBOOST_INCL := /Users/riveale/Downloads/boost_1_74_0
INC_FLAGS := -I. $(addprefix -I,$(INC_DIRS)) -I$(APPLE_OCV_INCL) -I$(APPLE_FFMPEG_INCL) -I$(FULLBOOST_INCL)

#REV: have to include min iphone OS version since earlier than 10 does not support C++17?
#REV: note that means that i386 processors or armv7 armv7s ;(

EXTRA := -MMD -MP -Wall -pthread -O2 -std=c++17 -miphoneos-version-min=13.4 -fembed-bitcode

#REV: all ipads are arm64..?
#REV: simulator must have platform simulator
platform := iphoneos
target := arm64

#platform := iphonesimulator
#target := x86_64 

#REV: I just realized that CPP means C preprocessor, not C plus plus lol
CC := `xcrun -sdk $(platform) -find clang`
CPP := $(CC) -E
CXX := $(CC)

sdkroot := `xcrun --sdk $(platform) --show-sdk-path`
AR = `xcrun -sdk $(platform) -find ar`
RANLIB :=`xcrun -sdk $(platform) -find ranlib`
LIPO := `xcrun -sdk $(platform) -find lipo`


CFLAGS := -arch $(target) -isysroot $(sdkroot) $(EXTRA)
CPPFLAGS := -arch $(target) -isysroot $(sdkroot) $(EXTRA)
LDFLAGS := -arch $(target) -isysroot $(sdkroot) $(EXTRA)

endif
############### END IOS ARM 64 ########################



OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: DOPRINT clean

DOPRINT:
	@echo "SRCS: ""$(SRCS)"
	@echo "Trying ""$(TARGET_EXEC)"



$(BUILD_BASE)/$(TARGET_EXEC): $(OBJS)
	$(CXX) $(INC_FLAGS) $(CPPFLAGS) $(OBJS) -o $@ $(LDFLAGS)


#$(BUILD_DIR)/./cpp/rtconnection.cpp.o: cpp/rtconnection.cpp $(INCLS)
#	$(MKDIR_P) $(dir $@)
#	$(CXX) $(INC_FLAGS) $(CPPFLAGS) -fno-rtti -c $< -o $@ $(LDFLAGS) 


$(BUILD_DIR)/%.cpp.o: %.cpp $(INCLS)
	$(MKDIR_P) $(dir $@)
	$(CXX) $(INC_FLAGS) $(CPPFLAGS) -c $< -o $@ $(LDFLAGS)



librteye2_ios.a: DOPRINT $(OBJS)
	$(AR) rvc $(BUILD_BASE)/$(TARGET_EXEC) $(OBJS)
	$(RANLIB) $(BUILD_BASE)/$(TARGET_EXEC)
#	$(LIPO) $(BUILD_BASE)/$(TARGET_EXEC)  -arch arm64  -output librteye2_ios_FINAL.a

librteye2.so: DOPRINT $(BUILD_BASE)/$(TARGET_EXEC)


clean:
	$(RM) -r $(BUILD_BASE)

-include $(DEPS)

MKDIR_P ?= mkdir -p

