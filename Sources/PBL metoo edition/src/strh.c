#include "types.h"
#include "strh.h"


size_t strlen(const char *ptr)
{
    unsigned long count;
    
    if (!ptr)
	return 0;

    for (count = 0; ptr[count] != 0; count++)
	;

    return count;
}

char *strh_get_token(char **ptr, char token)
{
    char *mark;
    char *s;
    
    s = mark = *ptr;

    for ( ; *s; s++) {
	if (*s == token) {
	    *s = '\0';
	    *ptr = ++s;
	    return mark;
	}
    }

    *ptr = s;

    return NULL;
}

char *strrchr(char *s, int c)
{
    char *last = NULL;

    for (; *s; s++)
	if (*s == (char)c)
	    last = s;

    return last;
}

int strncmp(const char *sz1, const char *sz2, size_t n)
{
    while (n > 0 && (*sz1) && (*sz2) && *sz1 == *sz2) {
	sz1++; sz2++; n--;
    }

    if (n == 0)
	return 0;

    return (*sz1 - *sz2);
}

int strcmp(const char *sz1, const char *sz2)
{
    while ((*sz1) && (*sz2) && *sz1 == *sz2) {
	sz1++; sz2++;
    }

    return (*sz1 - *sz2);
}

char *strh_eat_space(char *s)
{
    while (*s && isspace(*s))
	s++;
    
    return s;
}

char *strh_eat_nonspace(char *s)
{
    while (*s && !isspace(*s))
	s++;

    return s;
}

void strh_eat_trailing_space(char *s)
{
    char *ts = s;

    for (; *s; s++)
	if (!isspace(*s))
	    ts = s + 1;

    *ts = '\0';
}


char *strcpy(char *d, const char *s)
{
    char *dest = d;

    while (*s)
	*d++ = *s++;

    *d = '\0';

    return dest;
}

char *strncpy(char *d, const char *s, size_t n)
{
    char *dest = d;

    while (n > 0 && *s) {
	n--; *d++ = *s++;
    }

    if (n > 0)
	*d = '\0';

    return dest;
}

char *strh_dnzcpy(char *d, const char *s, size_t n)
{
    char *dest = d;

    if (n == 0)
	return NULL;

    while (n > 0 && *s) {
	n--; *d++ = *s++;
    }

    if (n > 0)
	*d = '\0';
    else
	*(d-1) = '\0';

    return dest;
}

char *strh_dnzcat(char *d, char *s, size_t n)
{
    char *dest = d;

    while (n > 0 && *d) {
	n--; d++;
    }

    strh_dnzcpy(d, s, n);

    return dest;
}

int isspace(char c)
{
    if (c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' ||
	c == '\v')
	return 1;

    return 0;
}

static char cvtIn[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,		/* '0' - '9' */
    100, 100, 100, 100, 100, 100, 100,		/* punctuation */
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,	/* 'A' - 'Z' */
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    30, 31, 32, 33, 34, 35,
    100, 100, 100, 100, 100, 100,		/* punctuation */
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,	/* 'a' - 'z' */
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    30, 31, 32, 33, 34, 35};

/*
 *----------------------------------------------------------------------
 *
 * strtoul --
 *
 *	Convert an ASCII string into an integer.
 *
 * Results:
 *	The return value is the integer equivalent of string.  If endPtr
 *	is non-NULL, then *endPtr is filled in with the character
 *	after the last one that was part of the integer.  If string
 *	doesn't contain a valid integer value, then zero is returned
 *	and *endPtr is set to string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

unsigned long strtoul(string, endPtr, base)
    char *string;		/* String of ASCII digits, possibly
				 * preceded by white space.  For bases
				 * greater than 10, either lower- or
				 * upper-case digits may be used.
				 */
    char **endPtr;		/* Where to store address of terminating
				 * character, or NULL. */
    int base;			/* Base for conversion.  Must be less
				 * than 37.  If 0, then the base is chosen
				 * from the leading characters of string:
				 * "0x" means hex, "0" means octal, anything
				 * else means decimal.
				 */
{
    unsigned char *p;
    unsigned long result = 0;
    unsigned int digit;
    int anyDigits = 0;

    /*
     * Skip any leading blanks.
     */

    p = string;
    while (isspace(*p)) {
	p += 1;
    }

    /*
     * If no base was provided, pick one from the leading characters
     * of the string.
     */
    
    if (base == 0)
    {
	if (*p == '0') {
	    p += 1;
	    if (*p == 'x') {
		p += 1;
		base = 16;
	    } else {

		/*
		 * Must set anyDigits here, otherwise "0" produces a
		 * "no digits" error.
		 */

		anyDigits = 1;
		base = 8;
	    }
	}
	else base = 10;
    } else if (base == 16) {

	/*
	 * Skip a leading "0x" from hex numbers.
	 */

	if ((p[0] == '0') && (p[1] == 'x')) {
	    p += 2;
	}
    }

    /*
     * Sorry this code is so messy, but speed seems important.  Do
     * different things for base 8, 10, 16, and other.
     */

    if (base == 8) {
	for ( ; ; p += 1) {
	    digit = *p - '0';
	    if (digit > 7) {
		break;
	    }
	    result = (result << 3) + digit;
	    anyDigits = 1;
	}
    } else if (base == 10) {
	for ( ; ; p += 1) {
	    digit = *p - '0';
	    if (digit > 9) {
		break;
	    }
	    result = (10*result) + digit;
	    anyDigits = 1;
	}
    } else if (base == 16) {
	for ( ; ; p += 1) {
	    digit = *p - '0';
	    if (digit > ('z' - '0')) {
		break;
	    }
	    digit = cvtIn[digit];
	    if (digit > 15) {
		break;
	    }
	    result = (result << 4) + digit;
	    anyDigits = 1;
	}
    } else {
	for ( ; ; p += 1) {
	    digit = *p - '0';
	    if (digit > ('z' - '0')) {
		break;
	    }
	    digit = cvtIn[digit];
	    if (digit >= base) {
		break;
	    }
	    result = result*base + digit;
	    anyDigits = 1;
	}
    }

    /*
     * See if there were any digits at all.
     */

    if (!anyDigits) {
	p = string;
    }

    if (endPtr != 0) {
	*endPtr = p;
    }

    return result;
}
