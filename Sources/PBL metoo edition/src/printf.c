#include "strh.h"
#include "vsprintf.c"
#include "ntfile.h"


extern unsigned char DebugFlag;


#define LOGNAME_BUF_SIZE 512

static int init_ok = 0;
static char log_file_name[LOGNAME_BUF_SIZE];



int printf_init(char *logfile, char *path)
{
    if (logfile && strlen(logfile) > 0) {
	strh_dnzcpy(log_file_name, logfile, LOGNAME_BUF_SIZE);
    } else {
	strh_dnzcpy(log_file_name, path, LOGNAME_BUF_SIZE);
	strh_dnzcat(log_file_name, "pbldebug.log", LOGNAME_BUF_SIZE);
    }

    init_ok = 1;
    return 1;
}


void printc(char c) {
    /* no output */
    return;
}


int print(const unsigned char* s, unsigned short len) {
    int i;
    for (i=0; i<len; i++) printc(s[i]);
    return 0;
}

static int print_to_file(const unsigned char* s, unsigned short len)
{
    if (init_ok)
	AppendFile(log_file_name, (PBYTE)s, len);
    
    return 0;
}

int printk(const char *fmt, ...) {
    char buf[512];
    unsigned short len;
    va_list argList;

    if (DebugFlag)
    {
	va_start(argList, fmt);
	len=(unsigned short) vsprintf(buf, fmt, argList);
	va_end(argList);
	return print_to_file(&buf[0], len);
    }
    return 0;
}
