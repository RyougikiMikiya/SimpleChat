APP_NAME  := SimpleChat
BUILD_OUT ?= build

#----------------------------------------------------------------
# Global Varibles & Compiler Refs -->
APP_CROSS_PATH  := 
LD_LIBRARY_PATH := $(APP_CROSS_PATH)/usr/lib
CROSS_ARCH      := 

ifeq ($(findstring $(APP_CROSS_PATH)/bin,$(PATH)),)
PATH  := $(PATH):$(APP_CROSS_PATH)/bin
endif

AR    := @$(CROSS_ARCH)ar
AS    := $(CROSS_ARCH)as
CC    := @$(CROSS_ARCH)gcc
CXX   := @$(CROSS_ARCH)g++
STRIP := @$(CROSS_ARCH)strip

CPU_TYPE := 
# <-- End: Global Varibles & Compiler Refs

#----------------------------------------------------------------
# Compiler Flags -->
# CXXFLAGS:
CXXFLAGS += -fexceptions
CXXFLAGS += -fstack-protector --param=ssp-buffer-size=4
CXXFLAGS += -fmessage-length=0
CXXFLAGS += -pipe
CXXFLAGS += -Wall
CXXFLAGS += -Wformat -Wformat-security
CXXFLAGS += -std=c++11

ifeq ($(RELEASE),release)
CXXFLAGS += -DNDEBUG
CXXFLAGS += -O2
else
CXXFLAGS += -g
endif
CFLAGS := $(CXXFLAGS)
CXXFLAGS += -fno-rtti

# LDFLAGS:
LDFLAGS  := -L$(APP_CROSS_PATH)/usr/lib
LDFLAGS  += -lpthread
LDFLAGS  += -lrt
# <-- End: Compiler Flags

#----------------------------------------------------------------
# env export -->
export PATH
export BUILD_OUT
export LANG AR AS CC CXX STRIP
export CXXFLAGS LDFLAGS
export LD_LIBRARY_PATH 
# <-- End: env export

#----------------------------------------------------------------
SOURCE_CPP := $(wildcard *.cpp)
CUR_OBJS   := $(patsubst %.cpp, $(BUILD_OUT)/%.o, $(SOURCE_CPP))
DEPS_CPP   := $(patsubst %.cpp, $(BUILD_OUT)/%.d, $(SOURCE_CPP))

SOURCE_C := $(wildcard *.c)
CUR_OBJS   += $(patsubst %.c, $(BUILD_OUT)/%.o, $(SOURCE_C))
DEPS_C   := $(patsubst %.c, $(BUILD_OUT)/%.d, $(SOURCE_C))
$(shell mkdir -p ${BUILD_OUT})

all:$(CUR_OBJS) $(APP_NAME)

$(BUILD_OUT)/%.d:%.c
	$(CC) $(CFLAGS) -MM $< -o $@
	@sed -i 's#\(.*\)\.o[ :]*#\1.o $@ : #g' $@

$(BUILD_OUT)/%.o:%.c $(BUILD_OUT)/%.d
	@echo "-->compiling $< ..."
	$(CC) $(CFLAGS) -c $< -o $@

sinclude $(DEPS_C)

$(BUILD_OUT)/%.d:%.cpp
	$(CXX) $(CXXFLAGS) -MM $< -o $@
	@sed -i 's#\(.*\)\.o[ :]*#\1.o $@ : #g' $@

$(BUILD_OUT)/%.o:%.cpp $(BUILD_OUT)/%.d
	@echo "-->compiling $< ..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

sinclude $(DEPS_CPP)

$(APP_NAME):
	$(CXX) -o $(BUILD_OUT)/$@ $(BUILD_OUT)/*.o $(LDFLAGS)
	@echo "------ Generated Binary File: <${APP_NAME}> Successed!!!"

ifeq ($(RELEASE),release)
	$(STRIP) $(BUILD_OUT)/$(APP_NAME)
endif

clean:
	-rm -rf $(BUILD_OUT)
	@echo "====== all generated files have been removed!!! Cleanup Done. "  

.PHONY: all clean

