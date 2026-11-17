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
