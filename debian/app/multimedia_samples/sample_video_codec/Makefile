ifeq ($(PARAM_FILE),)
    PARAM_FILE:=../Makefile.param
    include $(PARAM_FILE)
endif

INCDIR += -I$(CURDIR)/../../hbre/third_party/usr/include
LIBSDIR += -L$(CURDIR)/../../hbre/third_party/usr/lib

SRCS = $(wildcard *.c)

TARGET = $(patsubst %.c, %, ${SRCS})

.PHONY: all clean

%:%.c
	$(CC) -o $@ $^ $(INCDIR) ${CFLAGS} ${LIBSDIR} $(LDFLAGS)

all: ${TARGET}

install:
	@echo "Generate deployment package"
	@mkdir -p out
	@cp -arf ${TARGET} out/

clean:
	rm -rf ${OBJS} ${TARGET} out
