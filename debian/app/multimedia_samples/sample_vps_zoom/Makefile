ifeq ($(PARAM_FILE),) 
    PARAM_FILE:=../Makefile.param
    include $(PARAM_FILE)
endif

TARGET = sample_vps_zoom

SRC += main.c 

OBJS = $(SRC:.c=.o)

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
	@cp -arf ${TARGET} 19201080.yuv vps0_chn1_vps1_chn5_venc0.h264 out/

clean:
	rm -rf ${OBJS} ${TARGET} out
