ifeq ($(PARAM_FILE),) 
    PARAM_FILE:=../Makefile.param
    include $(PARAM_FILE)
endif

TARGET = sample_isp

SRC += main.c

OBJS = $(SRC:.c=.o)

.PHONY: all clean

%.o:%.c
	@mkdir -p $(abspath $(dir $@))
	$(CC) -o $@ $(INCDIR) -c $<

$(TARGET):$(OBJS)
	@mkdir -p $(abspath $(dir $@))
	$(CC) -o $@ $(OBJS) $(CFLAGS) $(LDFLAGS) ${LIBSDIR}
	
all: ${TARGET}

install:
	@echo "Generate deployment package"
	@mkdir -p out
	@cp -arf ${TARGET} out/

clean:
	rm -rf ${OBJS} ${TARGET} out
