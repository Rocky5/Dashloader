### compilers and options
CC	= gcc
CFLAGS	= -O2 -march=i686 -mcpu=i686 -Wall -Werror
CFLAGS += -DXBE
LD	= ld
LDFLAGS	= -s -S -T ldscript.ld
OBJCOPY	= objcopy
NASM	= nasm
NASMFLAGS = -f elf

### objects
OBJECTS	= header.o load.o video.o ntfile.o printf.o \
	cfgparser.o strh.o I2C_io.o


RESOURCES = 
TOPDIR  := $(shell /bin/pwd)

# target:
all	: clean image default.xbe

image:
	$(CC) $(TOPDIR)/imagebld/imagebld.c $(TOPDIR)/imagebld/sha1.c -o $(TOPDIR)/imagebld/image

default.elf : ${OBJECTS} ${RESOURCES}
	${LD} -o $@ ${OBJECTS} ${RESOURCES} ${LDFLAGS}

clean	:
	rm -rf *.o *~ core *.core image ${OBJECTS} ${RESOURCES} default.elf default.xbe $(TOPDIR)/imagebld/image

### rules:
%.o	: %.c
	${CC} ${CFLAGS} -o $@ -c $<

%.o	: %.S
	${CC} -DASSEMBLER ${CFLAGS} -o $@ -c $<

%.o	: %.asm
	${NASM} ${NASMFLAGS} -o $@ $<

%.xbe : %.elf
	${OBJCOPY} --output-target=binary --strip-all $< $@
	$(TOPDIR)/imagebld/image -build $(TOPDIR)/default.xbe  $(TOPDIR)/vmlinuz $(TOPDIR)/initrd  $(TOPDIR)/linuxboot.cfg
	@ls -l $@
