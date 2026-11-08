#include <stdint.h>

/* MiniRTOS V0: STM32F446RE, Cortex-M4, 16 MHz HSI, no FPU use. */
#define REG(a) (*(volatile uint32_t *)(a))
#define RCC_AHB1ENR REG(0x40023830u)
#define GPIOA_MODER REG(0x40020000u)
#define GPIOA_ODR   REG(0x40020014u)
#define SCB_VTOR    REG(0xE000ED08u)
#define SCB_ICSR    REG(0xE000ED04u)
#define SCB_SHPR3   REG(0xE000ED20u)
#define SYST_CSR    REG(0xE000E010u)
#define SYST_RVR    REG(0xE000E014u)
#define SYST_CVR    REG(0xE000E018u)
#define LED          (1u << 5)        /* LD2, PA5, active high */
#define STACK_WORDS  128u             /* 512 bytes per task */

extern uint32_t _estack, _sidata, _sdata, _edata, _sbss, _ebss;
void Reset_Handler(void);
void HardFault_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);
int main(void);

/* Flash vector table. The debugger can inspect these symbols. */
__attribute__((used, section(".isr_vector")))
void (*const vectors[16])(void) = {
    [0] = (void (*)(void))(&_estack),
    [1] = Reset_Handler,
    [3] = HardFault_Handler,
    [4] = HardFault_Handler, /* MemManage */
    [5] = HardFault_Handler, /* BusFault */
    [6] = HardFault_Handler, /* UsageFault */
    [14] = PendSV_Handler,
    [15] = SysTick_Handler,
};

void Reset_Handler(void) {
    uint32_t *src = &_sidata;
    for (uint32_t *p = &_sdata; p < &_edata;) *p++ = *src++;
    for (uint32_t *p = &_sbss; p < &_ebss;) *p++ = 0;
    SCB_VTOR = 0x08000000u;
    (void)main();
    for (;;) { }
}

void HardFault_Handler(void) { for (;;) { } }

volatile uint32_t ticks, counter_a, counter_b;
static uint32_t stack[2][STACK_WORDS] __attribute__((aligned(8)));
static uint32_t * volatile saved_sp[2] __attribute__((used));
static volatile uint32_t current __attribute__((used));

static void task_exit(void) { for (;;) { } }

static void create_task(unsigned id, void (*entry)(void)) {
    uint32_t *sp = stack[id] + STACK_WORDS;
    /* Cortex-M exception return stack frame, written from high to low. */
    *--sp = 0x01000000u;                 /* xPSR: Thumb state */
    *--sp = (uint32_t)(uintptr_t)entry & ~1u; /* PC: bit 0 must be clear */
    *--sp = (uint32_t)(uintptr_t)task_exit; /* LR */
    for (unsigned n = 0; n < 5; ++n) *--sp = 0; /* R12, R3-R0 */
    for (unsigned n = 0; n < 8; ++n) *--sp = 0; /* R11-R4 (software) */
    saved_sp[id] = sp;
}

/* PendSV runs at the lowest priority. PSP is private to each task. */
__attribute__((naked))
void PendSV_Handler(void) {
    __asm volatile (
        "mrs r0, psp                 \n"
        "cbz r0, 1f                 \n" /* no task to save on first switch */
        "stmdb r0!, {r4-r11}         \n"
        "ldr r1, =current           \n"
        "ldr r2, [r1]               \n"
        "ldr r3, =saved_sp          \n"
        "str r0, [r3, r2, lsl #2]   \n"
        "1:                          \n"
        "ldr r1, =current           \n"
        "ldr r2, [r1]               \n"
        "eor r2, r2, #1             \n" /* switch 0 <-> 1 */
        "str r2, [r1]               \n"
        "ldr r3, =saved_sp          \n"
        "ldr r0, [r3, r2, lsl #2]   \n"
        "ldmia r0!, {r4-r11}        \n"
        "msr psp, r0                \n"
        "ldr lr, =0xFFFFFFFD        \n" /* Thread mode, PSP */
        "bx lr                      \n"
    );
}

void SysTick_Handler(void) {
    ++ticks;
    SCB_ICSR = 1u << 28; /* pend PendSV */
}

static void task_a(void) {
    uint32_t last = 0;
    for (;;) {
        ++counter_a;
        if ((uint32_t)(ticks - last) >= 500u) {
            last += 500u;
            GPIOA_ODR ^= LED; /* toggle once every 500 ms */
        }
    }
}

static void task_b(void) {
    for (;;) ++counter_b;
}

int main(void) {
    RCC_AHB1ENR |= 1u;                     /* enable GPIOA clock */
    (void)RCC_AHB1ENR;                     /* allow clock to settle */
    GPIOA_MODER = (GPIOA_MODER & ~(3u << 10)) | (1u << 10);
    GPIOA_ODR &= ~LED;
    create_task(0, task_a);
    create_task(1, task_b);
    current = 1u;                          /* first switch selects task 0 */
    __asm volatile ("msr psp, %0" :: "r"(0u));
    SCB_SHPR3 = (0x80u << 24) | (0xFFu << 16); /* SysTick > PendSV */
    SYST_RVR = 16000u - 1u;               /* 1 ms at reset HSI 16 MHz */
    SYST_CVR = 0;
    SYST_CSR = 7u;                        /* CPU clock, IRQ, enable */
    SCB_ICSR = 1u << 28;                  /* start first task */
    for (;;) { }
}
