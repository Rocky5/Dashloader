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

#include <xtl.h>
#include "msxdk.h"
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include "external.h"
#include "dashloader_ui.h"

#define BUILD_VERSION "2.0.2"

#define INI_FILE "D:\\Dashloader.ini"

DASHLOADER_CONFIG g_cfg;

void __cdecl main()
{
	XMountRunningXBEDir();
	XMount("C:", "\\Device\\Harddisk0\\Partition2");

	// Load config from D:\Dashloader.ini (next to XBE)
	ini_load(&g_cfg, INI_FILE);
	g_cfg.UI_Enabled = 0;

	if (g_cfg.Log_Enabled)
		logfile = fopen("D:\\Dashloader.log", "w+t");

	debuglog("[Dashloader Bios Recovery Loader: v%s]", BUILD_VERSION);
	debuglog("Xbox kernel version: %d.%d.%d.%d", XboxKrnlVersion->VersionMajor, XboxKrnlVersion->VersionMinor, XboxKrnlVersion->Build, XboxKrnlVersion->Qfe);

	XInitDevices(0, NULL);
	if (FAILED(XBInput_CreateGamepads(&m_Gamepad)))
		debuglog("ERROR: Cant create gamepad");

	Sleep(300);
	BOOL buttonFired = FALSE;
	int timer = 0;
	while (timer++ <= 100)
	{
		XBInput_GetInput(m_Gamepad);

		ZeroMemory(&m_DefaultGamepad, sizeof(m_DefaultGamepad));
		for (DWORD i = 0; i < 4; i++)
		{
			if (m_Gamepad[i].hDevice)
			{
				m_DefaultGamepad.fX1 += m_Gamepad[i].fX1;
				m_DefaultGamepad.fY1 += m_Gamepad[i].fY1;
				m_DefaultGamepad.fX2 += m_Gamepad[i].fX2;
				m_DefaultGamepad.fY2 += m_Gamepad[i].fY2;
				m_DefaultGamepad.wButtons |= m_Gamepad[i].wButtons;
				m_DefaultGamepad.wPressedButtons |= m_Gamepad[i].wPressedButtons;
				m_DefaultGamepad.wLastButtons |= m_Gamepad[i].wLastButtons;
				for (DWORD b = 0; b < 8; b++)
				{
					m_DefaultGamepad.bAnalogButtons[b] |= m_Gamepad[i].bAnalogButtons[b];
					m_DefaultGamepad.bPressedAnalogButtons[b] |= m_Gamepad[i].bPressedAnalogButtons[b];
					m_DefaultGamepad.bLastAnalogButtons[b] |= m_Gamepad[i].bLastAnalogButtons[b];
				}
			}
		}

		if (!buttonFired && (m_DefaultGamepad.wButtons & XINPUT_GAMEPAD_BACK) && (m_DefaultGamepad.wButtons & XINPUT_GAMEPAD_START))
		{
			buttonFired = TRUE;
			g_cfg.UI_Enabled = 1;
			InitD3D();
			ToWide("Dashloader Bios Recovery Loader v" BUILD_VERSION, g_wszHeader, 64);
			ShowErrorScreen("Bios Recovery Mode...");
			Sleep(4000);
			debuglog("");
			debuglog("[Button Window]");
			debuglog("  Bios Recovery Mode");
			try_launch("Bios Recovery dashboard", "C:\\Bios loader\\recovery.xbe");
		}

		Sleep(1);
	}

	debuglog("");
	debuglog("[Bios Loader]");
	debuglog("  Bios loader");
	try_launch("Bios loader", "C:\\Bios loader\\Bios.xbe");

	debuglog("  Bios Recovery dashboard");
	try_launch("Bios Recovery dashboard", "C:\\Bios loader\\recovery.xbe");

	// If you're here show UI and count down
	debuglog("All failed. :( Insert a disc and load from there");
	if (!g_cfg.UI_Enabled)
		InitD3D();
	ToWide("Dashloader Bios Recovery Loader " BUILD_VERSION, g_wszHeader, 64);
	int failedTimer = 120;
	char msg[256];
	while (failedTimer >= 0)
	{
		_snprintf(msg, sizeof(msg), "All failed. :( Insert a disc and reboot.\nRebooting in %d", failedTimer);
		msg[sizeof(msg) - 1] = '\0';
		ShowErrorScreen(msg);
		Sleep(1000);
		failedTimer--;
	}
	fail_reboot();
}