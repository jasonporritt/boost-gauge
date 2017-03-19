#########
#
# The top level targets link in the two .o files for now.
#
TARGETS += jason-write

PIXELBONE_OBJS = PixelBone/pixel.o PixelBone/gfx.o PixelBone/matrix.o PixelBone/pru.o PixelBone/util.o
PIXELBONE_LIB := PixelBone/libpixelbone.a

all: $(TARGETS) PixelBone/ws281x.bin

CFLAGS += \
	-std=c99 \
	-W \
	-Wall \
	-D_BSD_SOURCE \
	-Wp,-MMD,$(dir $@).$(notdir $@).d \
	-Wp,-MT,$@ \
	-I. \
	-O2 \
	-mtune=cortex-a8 \
	-march=armv7-a \

LDFLAGS += \

LDLIBS += \
	-lpthread \

export CROSS_COMPILE:=

#####
#
# The TI "app_loader" is the userspace library for talking to
# the PRU and mapping memory between it and the ARM.
#
APP_LOADER_DIR ?= ./PixelBone/am335x/app_loader
APP_LOADER_LIB := $(APP_LOADER_DIR)/lib/libprussdrv.a
CFLAGS += -I$(APP_LOADER_DIR)/include
LDLIBS += $(APP_LOADER_LIB)

#####
#
# The TI PRU assembler looks like it has macros and includes,
# but it really doesn't.  So instead we use cpp to pre-process the
# file and then strip out all of the directives that it adds.
# PASM also doesn't handle multiple statements per line, so we
# insert hard newline characters for every ; in the file.
#
PASM_DIR ?= ./PixelBone/am335x/pasm
PASM := $(PASM_DIR)/pasm

%.bin: %.p $(PASM)
	$(CPP) - < $< | perl -p -e 's/^#.*//; s/;/\n/g; s/BYTE\((\d+)\)/t\1/g' > $<.i
	$(PASM) -V3 -b $<.i $(basename $@)
	$(RM) $<.i

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -std=c++11 `pkg-config --cflags --libs cairomm-1.0` -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<


$(foreach O,$(TARGETS),$(eval $O: $O.o $(PIXELBONE_OBJS) $(APP_LOADER_LIB)))

$(TARGETS):
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)


.PHONY: clean

clean:
	rm -rf \
		**/*.o \
		*.o \
		ws281x.hp.i \
		PixelBone/.*.o.d \
		*~ \
		$(INCDIR_APP_LOADER)/*~ \
		$(TARGETS) \
		*.bin \

###########
# 
# PRU Libraries and PRU assembler are build from their own trees.
# 
$(APP_LOADER_LIB):
	$(MAKE) -C $(APP_LOADER_DIR)/interface

$(PASM):
	$(MAKE) -C $(PASM_DIR)

# Include all of the generated dependency files
-include PixelBone/.*.o.d
