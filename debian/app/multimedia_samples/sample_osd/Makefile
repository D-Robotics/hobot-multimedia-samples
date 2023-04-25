ifeq ($(PARAM_FILE),) 
    PARAM_FILE:=../Makefile.param
    include $(PARAM_FILE)
endif

TARGET = sample_osd

SRC_PATH := $(CURDIR)

SRCS := $(foreach cf, $(SRC_PATH), $(wildcard $(cf)/*.c))
OBJS :=  $(SRCS:.c=.o)

.PHONY: all clean

%.o:%.c
	@mkdir -p $(abspath $(dir $@))
	$(CC) -o $@ $(INCDIR) -c $<

$(TARGET):$(OBJS)
	@mkdir -p $(abspath $(dir $@))
	$(CC) -o $@ $(OBJS) $(CFLAGS) ${LIBSDIR} $(LDFLAGS)

all: ${TARGET}

install:
	@echo "Generate deployment package"
	@mkdir -p out
	@cp -arf ${TARGET} 1280720.yuv out/

clean:
	rm -rf ${OBJS} ${TARGET} out
