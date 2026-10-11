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
    extern char kern_entry[];
    cprintf("Kernel entry at %p, kern_init at %p, bss end at %p\n",
            (void *)kern_entry, (void *)kern_init, (void *)end);
    
    cprintf("ucore shell ready:\n");
    while (1) {
        char *line = readline("> ");
        if (line != NULL && strlen(line) > 0)
            cprintf("cmd: %s\n", line);
    }


}
