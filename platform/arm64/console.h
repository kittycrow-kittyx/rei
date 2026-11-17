#ifndef REI_ARM64_CONSOLE_H
#define REI_ARM64_CONSOLE_H
void rei_console_init(void);
void rei_console_char(char c);
void rei_console_puts(const char *text);
void rei_console_number(unsigned long long x);
void rei_panic(const char *text);
#endif
