#include "xbox.h"
#include "xboxkrnl.h"
#include "strh.h"
#include "cfgparser.h"

extern unsigned char DebugFlag;

static int parse_rc4_key(UCHAR *d, char *s);
static int parse_eeprom_key(UCHAR *d, char *s);
static int parse_ledseq(UCHAR *d, char *s);
static int parse_config(char *path, char *buf, CONFIGENTRY *entry);



NTSTATUS cfgparser_get_config(CONFIGENTRY *entry)
{
    char path[MAX_PATHNAME];
    char filename[MAX_PATHNAME];
    char config[CONFIG_FILE_BUFSIZE];
    ANSI_STRING ConfigFileString;
    HANDLE ConfigFile;
    OBJECT_ATTRIBUTES ConfigFileAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Error;

    /* get the directory of the bootloader executable */

    memset(path, 0, MAX_PATHNAME);
    strncpy(path, XeImageFileName->Buffer,
	    XeImageFileName->Length < (MAX_PATHNAME - 1) ?
	    XeImageFileName->Length : (MAX_PATHNAME - 1));
    strrchr(path, '\\')[1] = 0;

    /* read the config file from there */

    strh_dnzcpy(filename, path, MAX_PATHNAME);
    strh_dnzcat(filename, CONFIG_FILE, MAX_PATHNAME);
    
    RtlInitAnsiString(&ConfigFileString, filename);
    
    ConfigFileAttributes.Attributes = OBJ_CASE_INSENSITIVE;
    ConfigFileAttributes.ObjectName = &ConfigFileString;
    ConfigFileAttributes.RootDirectory = NULL;
    
    Error = NtCreateFile(&ConfigFile,
			 0x80100080, /* GENERIC_READ |
					SYNCHRONIZE | FILE_READ_ATTRIBUTES */
			 &ConfigFileAttributes,
			 &IoStatusBlock,
			 NULL,
			 0,
			 7, /* FILE_SHARE_READ | FILE_SHARE_WRITE |
			       FILE_SHARE_DELETE */
			 1, /* FILE_OPEN */
			 0x60 /* FILE_NON_DIRECTORY_FILE |
				 FILE_SYNCHRONOUS_IO_NONALERT */
	);

    if (!NT_SUCCESS(Error))
	return Error;
    
    Error = NtReadFile(ConfigFile, NULL, NULL, NULL, &IoStatusBlock,
		       config, CONFIG_FILE_BUFSIZE - 2, NULL);

    if (!NT_SUCCESS(Error))
	return Error;

    config[IoStatusBlock.Information] = 0xA;
    config[IoStatusBlock.Information+1] = 0;
    
    parse_config(path, config, entry);
    
    return STATUS_SUCCESS;
}



static int parse_config(char *path, char *buf, CONFIGENTRY *entry)
{
    char *ptr;
    char *prm;
    unsigned int linenum;
    int ok;
    
    memset(entry, 0, sizeof(CONFIGENTRY));

    strh_dnzcpy(entry->path, path, MAX_PATHNAME);
    entry->err_linenum = 0;

    linenum = 0;

    for (;;) {
	linenum++;
	ptr = strh_get_token(&buf, 10);
	if (!ptr)
	    break;

	ptr = strh_eat_space(ptr);

	if (*ptr == '\0' || *ptr == '#')	/* empty or comment line */
	    continue;

	prm = strh_eat_nonspace(ptr);
	if (*prm != '\0') {
	    *prm++ = '\0';
	}
	prm = strh_eat_space(prm);
	strh_eat_trailing_space(prm);

	ok = TRUE;

	if (!strcmp(ptr, "Romfile")) {
	    strh_dnzcpy(entry->rom, path, MAX_PATHNAME);
	    strh_dnzcat(entry->rom, prm, MAX_PATHNAME);
	}
	else if (!strcmp(ptr, "AltRomfile")) {
	    strh_dnzcpy(entry->altrom, path, MAX_PATHNAME);
	    strh_dnzcat(entry->altrom, prm, MAX_PATHNAME);
	}
	else if (!strcmp(ptr, "RC4Key")) {
	    ok = parse_rc4_key(entry->szRC4Key, prm);
	}
	else if (!strcmp(ptr, "EEPROMKey1_0")) {
	    ok = parse_eeprom_key(entry->szEEPROMKey1_0, prm);
	}
	else if (!strcmp(ptr, "EEPROMKey1_1")) {
	    ok = parse_eeprom_key(entry->szEEPROMKey1_1, prm);
	}
	else if (!strcmp(ptr, "EEPROMKey1_6")) {
	    ok = parse_eeprom_key(entry->szEEPROMKey1_6, prm);
	}
	else if (!strcmp(ptr, "LEDSequence")) {
	    ok = parse_ledseq(&entry->ledseq, prm);
	    if (ok)
		entry->ledseq_set = 1;
	}
	else if (!strcmp(ptr, "Debug"))  {
	    if (!strcmp(prm, "true"))
		DebugFlag = TRUE;
	}
	else if (!strcmp(ptr, "DebugLog"))  {
	    strh_dnzcpy(entry->log, path, MAX_PATHNAME);
	    strh_dnzcat(entry->log, prm, MAX_PATHNAME);
	}
	else if (!strcmp(ptr, "Screen")) {
	    if (!strcmp(prm, "keep"))
		entry->screen = CFG_SCREEN_KEEP;
	    else if (!strcmp(prm, "off"))
		entry->screen = CFG_SCREEN_OFF;
	    else if (!strcmp(prm, "blank"))
		entry->screen = CFG_SCREEN_BLANK;
	    else if (!strcmp(prm, "nothing"))
		entry->screen = CFG_SCREEN_NOTHING;
	    else
		ok = FALSE;
	}
	else {
	    ok = FALSE;
	}

	if (!ok && entry->err_linenum == 0)
	    entry->err_linenum = linenum;
    }
    
    return (entry->err_linenum > 0 ? FALSE : TRUE);
}


static int parse_rc4_key(UCHAR *d, char *s)
{
    int i;

    memset(d, 0, 16);

    for (i = 0; i < 16; i++) {
	s = strh_eat_space(s);
	if (*s == '\0')
	    break;
	d[i] = (UCHAR)strtoul(s, &s, 0);
    }

    return TRUE;
}

static int parse_eeprom_key(UCHAR *d, char *s)
{
    return parse_rc4_key(d, s);
}

/*
 * Parse led color based on the sequence from the config file.
 * 'r' = red
 * 'g' = green
 * 'o' = orange
 * 'x' = off
 * This code is derived directly from the blink.c code
 *  (C) 2002-11-11 Georg Lukas <georg@boerde.de>
 */

static int parse_ledseq(UCHAR *d, char *s)
{
    int r, g;
    
    r = g = 0;

    while (*s) {
	r *= 2;
	g *= 2;

	if (*s == 'r') {
	    r++;
	} else if (*s == 'g') {
	    g++;
	} else if (*s == 'o') {
	    r++;
	    g++;
	}

	s++;
    }

    *d = (UCHAR)(((r << 4) & 0xF0) + (g & 0xF));

    return TRUE;
}
