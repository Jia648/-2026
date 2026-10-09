#include <sbi.h>
#include <defs.h>
#define SBI_SET_TIMER 0
#define SBI_CONSOLE_PUTCHAR 1
#define SBI_CONSOLE_GETCHAR 2
static unsigned long sbi_call(unsigned long sbi_type, unsigned long arg0, unsigned long arg1, unsigned long arg2) {
    unsigned long ret_val;
    __asm__ volatile (
        "mv x17, %[sbi_type]\n"
        "mv x10, %[arg0]\n"
        "mv x11, %[arg1]\n"
        "mv x12, %[arg2]\n"
        "ecall\n"
        "mv %[ret_val], x10"
        : [ret_val] "=r" (ret_val)
        : [sbi_type] "r" (sbi_type), [arg0] "r" (arg0), [arg1] "r" (arg1), [arg2] "r" (arg2)
        : "memory"
    );
    return ret_val;
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
