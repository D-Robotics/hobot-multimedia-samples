ifeq ($(PARAM_FILE),) 
    PARAM_FILE:=../Makefile.param
    include $(PARAM_FILE)
endif

TARGET = get_sif_data

SRC_PATH := $(CURDIR) $(CURDIR)/sensors
LDS_FILE = sensors.lds

SRCS := $(foreach cf, $(SRC_PATH), $(wildcard $(cf)/*.c))
OBJS :=  $(SRCS:.c=.o)

.PHONY: all clean

%.o:%.c
	@mkdir -p $(abspath $(dir $@))
	$(CC) -o $@ $(INCDIR) -c $<

$(TARGET):$(OBJS)
	@mkdir -p $(abspath $(dir $@))
	$(CC) -O3 -o $@ $(OBJS) $(LDS_FILE) $(CFLAGS) ${LIBSDIR} $(LDFLAGS)

all: ${TARGET}

install:
	@echo "Generate deployment package"
	@mkdir -p out
	@cp -arf ${TARGET} out/

clean:
	rm -rf ${OBJS} ${TARGET} out
