#include <sbi.h>
#include <defs.h>
#define SBI_SET_TIMER 0
#define SBI_CONSOLE_PUTCHAR 1
#define SBI_CONSOLE_GETCHAR 2
static unsigned long sbi_call(unsigned long sbi_type, unsigned long arg0, unsigned long arg1, unsigned long arg2) {
    register unsigned long a0 __asm__("a0") = arg0;
    register unsigned long a1 __asm__("a1") = arg1;
    register unsigned long a2 __asm__("a2") = arg2;
    register unsigned long a7 __asm__("a7") = sbi_type;
    __asm__ volatile (
        "ecall"
        : "+r" (a0)
        : "r" (a1), "r" (a2), "r" (a7)
        : "memory"
    );
    return a0;
}
void sbi_console_putchar(unsigned char ch) {
    sbi_call(SBI_CONSOLE_PUTCHAR, (unsigned long)ch, 0, 0);
}
int sbi_console_getchar(void) {
    return (int)sbi_call(SBI_CONSOLE_GETCHAR, 0, 0, 0);
}
void sbi_set_timer(unsigned long long stime_value) {
    sbi_call(SBI_SET_TIMER, (unsigned long)stime_value, 0, 0);
}
