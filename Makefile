CFLAGS = -W -Wall -pthread -g -pipe $(CFLAGS_EXTRA)
CFLAGS += -I inc -g 
RM = rm -rf
CC = g++
AR = ar
PREFIX?=/usr

# live555
ifneq ($(wildcard $(SYSROOT)/$(PREFIX)/include/liveMedia/liveMedia.hh),)
CFLAGS += -I $(SYSROOT)/$(PREFIX)/include/liveMedia  -I $(SYSROOT)/$(PREFIX)/include/groupsock -I $(SYSROOT)/$(PREFIX)/include/UsageEnvironment -I $(SYSROOT)/$(PREFIX)/include/BasicUsageEnvironment/
LDFLAGS += -L $(SYSROOT)/$(PREFIX)/lib -l:libliveMedia.a -l:libgroupsock.a -l:libUsageEnvironment.a -l:libBasicUsageEnvironment.a
else
$(error Cannot find live555)
endif

LIST_CPP:=$(wildcard src/*.cpp)
LIST_OBJ:=$(LIST_CPP:%.cpp=%.o)
LIB_NAME:=$(notdir $(CURDIR)).a

all: $(LIB_NAME) testRtspClient

%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_NAME): $(LIST_OBJ)
	$(AR) rcs $@ $^

testRtspClient: testRtspClient.cpp $(LIB_NAME)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean:
	-@$(RM) *.a $(LIST_OBJ) testRtspClient
