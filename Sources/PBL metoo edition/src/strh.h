#ifndef _strh_h_
#define _strh_h_

#include "types.h"


size_t strlen(const char *ptr);
char *strh_get_token(char **ptr, char token);
char *strrchr(char *s, int c);
int strncmp(const char *sz1, const char *sz2, size_t n);
int strcmp(const char *sz1, const char *sz2);
char *strcpy(char *d, const char *s);
char *strncpy(char *d, const char *s, size_t n);
char *strh_dnzcpy(char *d, const char *s, size_t n);
char *strh_dnzcat(char *d, char *s, size_t n);
unsigned long strtoul(char *string, char **endPtr, int base);
int isspace(char c);
char *strh_eat_space(char *s);
char *strh_eat_nonspace(char *s);
void strh_eat_trailing_space(char *s);


/* memcpy and memset originally from Linux kernel */

static inline void * memcpy(void * to, const void * from, unsigned long n)
{
int d0, d1, d2;
__asm__ __volatile__(
        "push %%ecx\n\t"
	"shr $2,%%ecx\n\t"
        "rep ; movsl\n\t"
        "pop %%ecx\n\t"
        "and $3,%%ecx\n\t"
        "rep ; movsb\n\t"
        : "=&c" (d0), "=&D" (d1), "=&S" (d2)
        : "0" (n), "1" ((long) to), "2" ((long) from)
        : "memory");
return (to);
}

static inline void * memset(void * s, int c, unsigned long count)
{
int d0, d1;
__asm__ __volatile__(
        "push %%ecx\n\t"
	"shr $2,%%ecx\n\t"
        "rep ; stosl\n\t"
        "pop %%ecx\n\t"
        "and $3,%%ecx\n\t"
        "rep ; stosb\n\t"
        : "=&c" (d0), "=&D" (d1)
        : "a" (0x01010101*(unsigned char)c), "0" (count), "1" ((long) s)
        : "memory");
return (s);     
}


#endif /* _strh_h_ */
