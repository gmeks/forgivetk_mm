#(C)2004-2005 SourceMM Development Team
# Makefile written by David "BAILOPAN" Anderson

HL2SDK = ../sdk/HL2Sdk
SMM_ROOT = ../sdk/Sourcemm
SRCDS = ../srcds

#HL2SDK = ../../../MyMpd/src
#SMM_ROOT = ../..
#SRCDS = /cygdrive/c/srcds

### EDIT BELOW FOR OTHER PROJECTS ###

OPT_FLAGS = -O3 -fno-rtti -funroll-loops -s -pipe
DEBUG_FLAGS = -g -ggdb3
CPP = ../opt/crosstool/gcc-3.4.1-glibc-2.3.2/i686-unknown-linux-gnu/bin/i686-unknown-linux-gnu-gcc
#CPP = /opt/crosstool/gcc-3.4.1-glibc-2.3.2/i686-unknown-linux-gnu/bin/i686-unknown-linux-gnu-gcc

BINARY = forgivetk_mm_i486.so

OBJECTS = ForgiveTK.cpp cvars.cpp SharedFunctions.cpp ForgiveTKMenu.cpp BATMenu.cpp Utils.cpp ModInfo.cpp Utils.cpp \
	hl2sdk/recipientfilters.cpp 

LINK = $(HL2SDK)/linux_sdk/tier1_i486.a vstdlib_i486.so

HL2PUB = $(HL2SDK)/public

INCLUDE = -I. -I$(HL2PUB) -I$(HL2PUB)/dlls -I$(HL2PUB)/engine -I$(HL2PUB)tier0 -I$(HL2PUB)/tier1 \
-I$(HL2PUB)/vstdlib -I$(HL2SDK)/tier1  -I$(HL2SDK)/game_shared -I$(HL2SDK)/dlls \
-I$(SMM_ROOT) -I$(SMM_ROOT)/sourcehook  -I$(SMM_ROOT)/sourcemm

ifeq "$(DEBUG)" "true"
	BIN_DIR = Debug
	CFLAGS = $(DEBUG_FLAGS)
else
	BIN_DIR = Release
	CFLAGS = $(OPT_FLAGS)
endif

CFLAGS += -fpermissive -D_LINUX -DNDEBUG -Dstricmp=strcasecmp -D_stricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf -D_vsnprintf=vsnprintf -D_alloca=alloca -Dstrcmpi=strcasecmp -fPIC -Wno-deprecated -msse

OBJ_LINUX := $(OBJECTS:%.cpp=$(BIN_DIR)/%.o)

$(BIN_DIR)/%.o: %.cpp
	$(CPP) $(INCLUDE) $(CFLAGS) -o $@ -c $<

all:
	mkdir -p $(BIN_DIR)
	mkdir -p $(BIN_DIR)/hl2sdk
	mkdir -p $(BIN_DIR)/csdm
	ln -sf $(SRCDS)/bin/vstdlib_i486.so vstdlib_i486.so
	ln -sf $(SRCDS)/bin/tier0_i486.so tier0_i486.so
	$(MAKE) sourcemm

sourcemm: $(OBJ_LINUX)
	$(CPP) $(INCLUDE) $(CFLAGS) $(OBJ_LINUX) $(LINK) -shared -ldl -lm -o$(BIN_DIR)/$(BINARY)

debug:	
	$(MAKE) all DEBUG=true

default: all

clean:
	rm -rf Release/*.o
	rm -rf Release/hl2sdk/*.o
	rm -rf Release/csdm/*.o
	rm -rf Release/$(BINARY)
	rm -rf Debug/*.o
	rm -rf Debug/$(BINARY)
