#include <stdint.h>
#include "console.h"
#if defined(REI_PI5)
#define UART_BASE UINT64_C(0x107d001000)
#else
#define UART_BASE UINT64_C(0x3f201000)
#endif
static volatile uint32_t *reg(unsigned off) { return (volatile uint32_t *)(uintptr_t)(UART_BASE+off); }
void rei_console_init(void) {
    *reg(0x30)=0; *reg(0x24)=26; *reg(0x28)=3; *reg(0x2c)=0x70;
    *reg(0x38)=0; *reg(0x44)=0x7ff; *reg(0x30)=0x301;
}
void rei_console_char(char c) {
    if(c=='\n')rei_console_char('\r');
    for(unsigned i=0;i<1000000;i++){
        if(!(*reg(0x18)&(1u<<5))){*reg(0)=(unsigned char)c;break;}
    }
}
void rei_console_puts(const char *s){while(*s)rei_console_char(*s++);}
void rei_console_number(unsigned long long x){char digits[21];unsigned n=0;do{digits[n++]=(char)('0'+x%10);x/=10;}while(x);while(n)rei_console_char(digits[--n]);}
void rei_panic(const char *s){rei_console_puts(s);for(;;)__asm__ volatile("wfe");}

static void hex64(uint64_t v) {
    for (unsigned i = 64; i; i -= 4) rei_console_char("0123456789abcdef"[(v >> (i-4)) & 15]);
}
__attribute__((noreturn)) void rei_exception(uint64_t esr, uint64_t elr, uint64_t far, uint64_t el) {
    rei_console_puts("\nREI: ARM EXCEPTION EL="); rei_console_number(el >> 2);
    rei_console_puts(" ESR=0x"); hex64(esr);
    rei_console_puts(" ELR=0x"); hex64(elr);
    rei_console_puts(" FAR=0x"); hex64(far);
    rei_panic("\n");
    for (;;) __asm__ volatile("wfe");
}

int rei_console_read(void) {
    if (*reg(0x18) & (1u << 4)) return -1;
    uint32_t data = *reg(0);
    return (data & 0xf00u) ? -1 : (int)(data & 0xffu);
}
void rei_console_poll(void) {}
void rei_console_idle(void) { for (;;) __asm__ volatile("wfe"); }
