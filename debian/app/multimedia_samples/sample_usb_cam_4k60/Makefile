ifeq ($(PARAM_FILE),)
    PARAM_FILE:=../Makefile.param
    include $(PARAM_FILE)
endif

TARGET = sample_usb_cam

SRC_PATH := $(CURDIR) $(CURDIR)/src/camera $(CURDIR)/src/utils $(CURDIR)/src/vio
INCDIR += -I$(CURDIR)/inc
INCDIR += -I$(CURDIR)/inc/vio
SRCS := $(foreach cf, $(SRC_PATH), $(wildcard $(cf)/*.c))
SRCS_cpp := $(foreach cf, $(SRC_PATH), $(wildcard $(cf)/*.cpp))
OBJS := $(SRCS:.c=.o) $(SRCS_cpp:.cpp=.obj)
LIBSDIR += -L$(CURDIR)/lib/jsoncpp/
LDFLAGS += -lguvc \
	-ljsoncpp

.PHONY: all clean

%.o:%.c
	@mkdir -p $(abspath $(dir $@))
	$(CC) -o $@ -MMD -MP $(INCDIR) -c $<

%.obj:%.cpp
	@mkdir -p $(abspath $(dir $@))
	$(CPP) -o $@ -MMD -MP $(INCDIR) -c $<

$(TARGET):$(OBJS)
	@mkdir -p $(abspath $(dir $@))
	$(CC) -o $@  -lstdc++ $(OBJS) $(CFLAGS) ${LIBSDIR} $(LDFLAGS)

all: ${TARGET}

install:
	@echo "Generate deployment package"
	@mkdir -p out
	@cp -arf ${TARGET} out/
	@cp -arf *.sh out/
	@cp -arf *.json out/
	@cp -arf *.so out/

clean:
	rm -rf ${OBJS} ${TARGET} out
	rm -f `find -name "*.d"`
