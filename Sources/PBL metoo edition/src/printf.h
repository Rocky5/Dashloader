#ifndef _PRINT_H_
#define _PRINT_H_

int printf_init(char *logfile, char *path);
void printc(char c);
int print(const unsigned char* s, unsigned short len);
int printk(const char *fmt, ...);
#define dprintf printk

#endif
