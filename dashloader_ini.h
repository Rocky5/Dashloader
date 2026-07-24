/*
*      Copyright (C) 2026 Rocky5
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*/

#ifndef DASHLOADER_H
#define DASHLOADER_H
#define MAX_PATH_LEN 256

typedef struct _DASHLOADER_CONFIG
{
	// [UI]
	int UI_Enabled;
	int UI_SingleColour;
	int UI_ButtonDelay;
	int UI_LaunchDelay;

	// [Logging]
	int Log_Enabled;

	// [Buttons]
	char Btn_A[MAX_PATH_LEN];
	char Btn_B[MAX_PATH_LEN];
	char Btn_X[MAX_PATH_LEN];
	char Btn_Y[MAX_PATH_LEN];
	char Btn_Start[MAX_PATH_LEN];
	char Btn_Back[MAX_PATH_LEN];
	char Btn_Black[MAX_PATH_LEN];
	char Btn_White[MAX_PATH_LEN];

	// [Recovery]
	char Recovery_Path[MAX_PATH_LEN];

	// [Dashboard]
	char Dashboard_Path[MAX_PATH_LEN];

	// [VirtualDrive]
	int VirtualDrive_ISOKernelPatch;
	int VirtualDrive_DismountISOonIGR;

} DASHLOADER_CONFIG;


static void ini_trim(char* s)
{
	int start = 0;
	int len = strlen(s);
	int end = len - 1;

	while (start < len && (s[start] == ' ' || s[start] == '\t'))
		start++;
	while (end >= start && (s[end] == ' ' || s[end] == '\t' || s[end] == '\r' || s[end] == '\n'))
		end--;

	int newlen = end - start + 1;
	if (newlen <= 0)
	{
		s[0] = '\0';
		return;
	}
	memmove(s, s + start, newlen);
	s[newlen] = '\0';
}


static void ini_set(DASHLOADER_CONFIG* cfg, const char* key, const char* val)
{
	if (_stricmp(key, "UI_Enabled") == 0)
		cfg->UI_Enabled = atoi(val);
	else if (_stricmp(key, "UI_SingleColour") == 0)
		cfg->UI_SingleColour = atoi(val);
	else if (_stricmp(key, "UI_ButtonDelay") == 0)
		cfg->UI_ButtonDelay = atoi(val);
	else if (_stricmp(key, "UI_LaunchDelay") == 0)
		cfg->UI_LaunchDelay = atoi(val);
	else if (_stricmp(key, "Logging_Enabled") == 0)
		cfg->Log_Enabled = atoi(val);
	else if (_stricmp(key, "Buttons_A") == 0)
		strncpy(cfg->Btn_A, val, MAX_PATH_LEN - 1);
	else if (_stricmp(key, "Buttons_B") == 0)
		strncpy(cfg->Btn_B, val, MAX_PATH_LEN - 1);
	else if (_stricmp(key, "Buttons_X") == 0)
		strncpy(cfg->Btn_X, val, MAX_PATH_LEN - 1);
	else if (_stricmp(key, "Buttons_Y") == 0)
		strncpy(cfg->Btn_Y, val, MAX_PATH_LEN - 1);
	else if (_stricmp(key, "Buttons_Start") == 0)
		strncpy(cfg->Btn_Start, val, MAX_PATH_LEN - 1);
	else if (_stricmp(key, "Buttons_Back") == 0)
		strncpy(cfg->Btn_Back, val, MAX_PATH_LEN - 1);
	else if (_stricmp(key, "Buttons_Black") == 0)
		strncpy(cfg->Btn_Black, val, MAX_PATH_LEN - 1);
	else if (_stricmp(key, "Buttons_White") == 0)
		strncpy(cfg->Btn_White, val, MAX_PATH_LEN - 1);
	else if (_stricmp(key, "Recovery_Path") == 0)
		strncpy(cfg->Recovery_Path, val, MAX_PATH_LEN - 1);
	else if (_stricmp(key, "Dashboard_Path") == 0)
		strncpy(cfg->Dashboard_Path, val, MAX_PATH_LEN - 1);
	else if (_stricmp(key, "VirtualDrive_ISOKernelPatch") == 0)
		cfg->VirtualDrive_ISOKernelPatch = atoi(val);
	else if (_stricmp(key, "VirtualDrive_DismountISOonIGR") == 0)
		cfg->VirtualDrive_DismountISOonIGR = atoi(val);
}


static void ini_load(DASHLOADER_CONFIG* cfg, const char* ini_path)
{
	// Defaults
	cfg->UI_Enabled = 1;
	cfg->UI_SingleColour = 0;
	cfg->UI_ButtonDelay = 1000;
	cfg->UI_LaunchDelay = 700;
	cfg->Log_Enabled = 1;
	cfg->Btn_A[0] = '\0';
	cfg->Btn_B[0] = '\0';
	cfg->Btn_X[0] = '\0';
	cfg->Btn_Y[0] = '\0';
	cfg->Btn_Start[0] = '\0';
	cfg->Btn_Back[0] = '\0';
	cfg->Btn_Black[0] = '\0';
	cfg->Btn_White[0] = '\0';
	cfg->Recovery_Path[0] = '\0';
	cfg->Dashboard_Path[0] = '\0';
	cfg->VirtualDrive_ISOKernelPatch = 0;
	cfg->VirtualDrive_DismountISOonIGR = 0;

	FILE* f = fopen(ini_path, "r");
	if (!f)
		return;

	char line[512];
	char section[64] = "";

	while (fgets(line, sizeof(line), f))
	{
		ini_trim(line);

		// Skip empty lines and comments
		if (line[0] == '\0' || line[0] == ';' || line[0] == '#')
			continue;

		// Section header, store for key prefixing
		if (line[0] == '[')
		{
			int len = strlen(line);
			if (line[len - 1] == ']')
			{
				strncpy(section, line + 1, len - 2);
				section[len - 2] = '\0';
				ini_trim(section);
			}
			continue;
		}

		// Key = Value
		char* eq = strchr(line, '=');
		if (!eq)
			continue;

		*eq = '\0';
		char key[128];
		char val[MAX_PATH_LEN];

		strncpy(key, line, sizeof(key) - 1);
		key[sizeof(key) - 1] = '\0';
		ini_trim(key);

		strncpy(val, eq + 1, sizeof(val) - 1);
		val[sizeof(val) - 1] = '\0';
		ini_trim(val);

		char* inline_comm = strchr(val, ';');
		if (!inline_comm) inline_comm = strchr(val, '#');
		if (inline_comm) {
			*inline_comm = '\0';
			ini_trim(val);
		}

		// Build key: Section_Key
		char fullkey[MAX_PATH_LEN];
		if (section[0])
			sprintf(fullkey, "%s_%s", section, key);
		else
			strncpy(fullkey, key, sizeof(fullkey) - 1);

		ini_set(cfg, fullkey, val);
	}

	fclose(f);
}

#endif