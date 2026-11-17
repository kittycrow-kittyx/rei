#ifndef REI_ARM64_CONSOLE_H
#define REI_ARM64_CONSOLE_H
#ifdef __cplusplus
extern "C" {
#endif
void rei_console_init(void);
void rei_console_char(char c);
void rei_console_puts(const char *text);
void rei_console_number(unsigned long long x);
int rei_console_read(void);
void rei_console_poll(void);
void rei_console_idle(void);
void rei_panic(const char *text);
#ifdef __cplusplus
}
#endif
#endif
