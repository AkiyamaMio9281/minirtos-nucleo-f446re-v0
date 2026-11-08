# MiniRTOS V0 — NUCLEO-F446RE

A very small, bare-metal, preemptive two-task kernel for STM32F446RE (ARM
Cortex-M4). It runs from the chip's internal HSI clock at its reset default of
16 MHz, with a 1 ms SysTick and PendSV round-robin context switching.

No HAL, CMSIS package, libc, C runtime, malloc or vendor-generated files.
A small amount of ARM Thumb-2 assembly is necessary for context switching.

## What it does

- Two tasks, each with a private 512-byte stack.
- Task A increments `counter_a` and toggles the **green LD2 LED (PA5)**
  every 500 SysTick ticks. LED should blink ~1 Hz (on/off cycle).
- Task B increments `counter_b` continuously.
- `ticks` increments approximately 1000 times per second.
- Both tasks are preempted by the timer, regardless of whether they yield.

## Prerequisites

- NUCLEO-F446RE connected by the **ST-LINK USB connector**.
- ARM GNU Toolchain (`arm-none-eabi-gcc`), GNU Make, OpenOCD.
- Optional: `arm-none-eabi-gdb` or `gdb-multiarch`.

## Build and flash

```sh
make
make flash
```

`make flash` uses OpenOCD's `board/st_nucleo_f4.cfg` with the onboard ST-LINK
probe. The board should reset into the firmware. Expected behavior: LD2
blinks about once per second. This is a prediction, not a result of an
on-board test.

Alternative compile test with LLVM/Clang and LLD:

```sh
make TOOLCHAIN=clang
```

## Debug

Terminal A:

```sh
make debug-server
```

Terminal B:

```sh
arm-none-eabi-gdb minirtos.elf
(gdb) target extended-remote :3333
(gdb) monitor reset halt
(gdb) break HardFault_Handler
(gdb) continue
# After a few seconds, interrupt execution with Ctrl+C.
(gdb) p ticks
(gdb) p counter_a
(gdb) p counter_b
(gdb) p current
```

Expected: `ticks`, `counter_a`, and `counter_b` are nonzero and grow on
successive checks. `current` alternates between 0 and 1 at the 1 ms tick.
Check at multiple breakpoints/samples; it is not guaranteed to differ on any
two arbitrarily timed samples.

## Why it is specific to this board

- Cortex-M4 `-mcpu=cortex-m4`, soft float to avoid FPU context handling.
- Flash `0x08000000` / 512 KiB and SRAM `0x20000000` / 128 KiB.
- LD2 connected to STM32 GPIO PA5 (output high means on).
- RCC/GPIO registers are those of the STM32F446 family.
- The vector table is placed at Flash base; `VTOR` is set accordingly.
- No PLL setup: assumes a full reset and the default HSI 16 MHz CPU clock.
  If you later configure PLL/HCLK, update SysTick reload accordingly.

## Limitations (intentional in V0)

- No sleep, semaphore, mutex, task creation at runtime, or priority scheduler.
- Only 2 tasks. Both busy-loop and must never return.
- No stack overflow checking or interrupt-safe application API.
- Floating-point instructions are disabled by the build ABI; to enable FPU
  use in tasks later you must handle the additional floating-point context.
- The firmware is compiled and inspected, **not tested on a physical board**.

## Next steps

V1: voluntary `yield()`; V2: a task control block and N-task scheduler;
V3: delay/sleep; V4: semaphore; V5: mutex; V6: message queue/IPC.
