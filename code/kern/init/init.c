#include <stdio.h>
#include <string.h>
#include <sbi.h>

int kern_init(void) __attribute__((noreturn));

int kern_init(void) {
    extern char edata[], end[];
    memset(edata, 0, end - edata);
    
    cprintf("(THU.CST) os is loading ...\n");
    cprintf("Hello uCore!\n");
    cprintf("Test printf: decimal=%d, hex=0x%x, char=%c, string=%s\n", 12345, 0xdeadbeef, 'A', "test ok");
    cprintf("Kernel entry at 0x%x, bss end at 0x%x\n", kern_init, end);
    
        cprintf("ucore shell ready:\n");
    while (1) {
        char *line = readline("> ");
        if (strlen(line) > 0)
            cprintf("cmd: %s\n", line);
    }


}
