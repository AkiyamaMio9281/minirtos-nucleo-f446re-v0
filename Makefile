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

# STM32CubeProgrammer CLI: the copy bundled with STM32CubeIDE if present,
# otherwise the one on PATH. Override with: make flash CUBEPROG=<path>
CUBEPROG_IDE := $(lastword $(sort $(wildcard C:/ST/STM32CubeIDE_*/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_*/tools/bin/STM32_Programmer_CLI.exe)))
CUBEPROG ?= $(or $(CUBEPROG_IDE),STM32_Programmer_CLI)

all: minirtos.elf
minirtos.elf: main.c linker.ld
	$(CC) $(CFLAGS) $(LDFLAGS) main.c -o $@
flash: minirtos.elf
	$(CUBEPROG) -c port=SWD mode=UR -d minirtos.elf -v -rst
debug-server:
	openocd -f board/st_nucleo_f4.cfg
clean:
	rm -f minirtos.elf
.PHONY: all flash debug-server clean
