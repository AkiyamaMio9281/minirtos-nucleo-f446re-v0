TOOLCHAIN ?= gcc
ifeq ($(TOOLCHAIN),clang)
CC = clang
EXTRA = --target=arm-none-eabi -fuse-ld=lld
else
CC = arm-none-eabi-gcc
endif
CFLAGS = $(EXTRA) -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Os -g \
         -ffreestanding -fno-builtin -fno-unwind-tables \
         -ffunction-sections -fdata-sections -Wall -Wextra
LDFLAGS = -nostdlib -Wl,-T,linker.ld -Wl,--gc-sections

all: minirtos.elf
minirtos.elf: main.c linker.ld
	$(CC) $(CFLAGS) $(LDFLAGS) main.c -o $@
flash: minirtos.elf
	openocd -f board/st_nucleo_f4.cfg -c 'program minirtos.elf verify reset exit'
debug-server:
	openocd -f board/st_nucleo_f4.cfg
clean:
	rm -f minirtos.elf
.PHONY: all flash debug-server clean
