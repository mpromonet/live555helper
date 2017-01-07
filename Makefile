CFLAGS = -W -Wall -pthread -g -pipe $(CFLAGS_EXTRA)
CFLAGS += -I inc
RM = rm -rf
CC = gcc
AR = ar

# live555
ifneq ($(wildcard $(SYSROOT)/usr/include/liveMedia/liveMedia.hh),)
	CFLAGS += -I $(SYSROOT)/usr/include/liveMedia  -I $(SYSROOT)/usr/include/groupsock -I $(SYSROOT)/usr/include/UsageEnvironment -I $(SYSROOT)/usr/include/BasicUsageEnvironment/
	LDFLAGS += -l:libliveMedia.a -l:libgroupsock.a -l:libUsageEnvironment.a -l:libBasicUsageEnvironment.a
else
	$(error Cannot find live555)
endif

LIST_CPP:=$(wildcard src/*.cpp)
LIST_OBJ:=$(LIST_CPP:%.cpp=%.o)
LIB_NAME:=$(notdir $(CURDIR)).a

all: $(LIB_NAME)

%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

$(LIB_NAME): $(LIST_OBJ)
	$(AR) rcs $@ $^


clean:
	-@$(RM) *.a $(LIST_OBJ)
