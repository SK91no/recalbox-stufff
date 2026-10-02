CC=gcc
CFLAGS=-I ./ -I $(CROSS_DIR)/usr/include/drm -Wall -Werror
LDFLAGS=-lmosquitto -ldrm
OBJS=main.o ssd1306.o mqtt.o kms-mode.o log.o
BIN=recalboxrgbjamma_screen

ifneq ("$(ARCH_RPI4)", "")
	CFLAGS+=-DARCH_RPI4=1
endif
ifneq ("$(ARCH_RPI3)", "")
	CFLAGS+=-DARCH_RPI3=1
endif

default: $(BIN)
.PHONY: default clean

# Adapted from scottmcpeak.com/autodepend/autodepend.html
-include $(OBJS:.o=.d)
%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $*.o
	$(CC) -MM $(CFLAGS) $< > $*.d
	@cp -f $*.d $*.d.tmp
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

$(BIN):$(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

clean:
	rm -f *.o *.d $(BIN)
