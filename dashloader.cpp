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
#include <fstream>
#include "kernelpatcher.h"
#include "external.h"
#include "dashloader_ui.h"

#define BUILD_VERSION "2.1.4"

#define INI_FILE         "D:\\Dashloader.ini"

#define ES_IGR           "E:\\CACHE\\LocalCache20.bin"
#define PATCHER_FILE     "E:\\CACHE\\LocalCache40.bin"

#define PATCHED_IND      ((char*)0x8002B4B7)
#define PATCHED_M8       ((char*)0x8002691E)
#define M8_CHECK1        ((char*)0x80026919)
#define M8_CHECK2        ((char*)0x8002691D)

DASHLOADER_CONFIG g_cfg;

char xbeName[64] = "";
int debuggingDelay = 0; // use 1000 or 2000 when testing
int softmodded = 0;
unsigned long IOCTL_VIRTUAL_CDROM;

void __cdecl main()
{
	XMountRunningXBEDir();
	// Don't need to mount C and E but just in the off change pigs fly
	XMount("C:", "\\Device\\Harddisk0\\Partition2");
	XMount("E:", "\\Device\\Harddisk0\\Partition1");
	XMount("F:", "\\Device\\Harddisk0\\Partition6");
	XMount("R:", "\\Device\\Harddisk0\\Partition14");

	// Check if we are running my softmod. (aka Rocky5 softmod)
	softmodded = file_exist("R:\\nkpatcher\\default.xbe");

	CreateDirectory("E:\\CACHE", NULL);

	// Load config from D:\Dashloader.ini (next to XBE)
	ini_load(&g_cfg, INI_FILE);

	if (g_cfg.Log_Enabled)
		logfile = fopen("D:\\Dashloader.log", "w+t");

	XGetXBEName(xbeName, sizeof(xbeName));
	WORD kernelBuild = XboxKrnlVersion->Build;
	debuglog("[Dashloader: v%s]", BUILD_VERSION);
	debuglog("Running XBE: %s", xbeName);
	debuglog("Xbox kernel version: %d.%d.%d.%d", XboxKrnlVersion->VersionMajor, XboxKrnlVersion->VersionMinor, kernelBuild, XboxKrnlVersion->Qfe);

	if (g_cfg.UI_Enabled)
	{
		InitD3D();
		ToWide("Dashloader v" BUILD_VERSION, g_wszHeader, 64);
		Sleep(debuggingDelay);
	}

	XInitDevices(0, NULL);
	if (FAILED(XBInput_CreateGamepads(&m_Gamepad)))
		debuglog("ERROR: Cant create gamepad");

	// Ind-Bios 5003 / M8+ virtual disc loader patch
	if (!g_cfg.VirtualDrive_ISOKernelPatch)
	{
		// Ind-Bios = 5003 / M8+ = 5838
		// 0x55 = U & 0xEC = ì / é & . are at the same offsets in stock 1.6 kernel, prevents crash.
		bool isIndpatched = kernelBuild == 5003 && (unsigned char)*PATCHED_IND == 0x99;
		bool isIndUnpatched = kernelBuild == 5003 && (unsigned char)*PATCHED_IND != 0x99;

		bool isM8patched = kernelBuild == 5838 && (unsigned char)*PATCHED_M8 == 0x99 && (unsigned char)*M8_CHECK1 == 0x55 && (unsigned char)*M8_CHECK2 == 0xEC;
		bool isM8Unpatched = kernelBuild == 5838 && (unsigned char)*PATCHED_M8 != 0x99 && (unsigned char)*M8_CHECK1 == 0x55 && (unsigned char)*M8_CHECK2 == 0xEC;

		if (isM8patched)
		{
			debuglog("");
			debuglog("[Bios]");
			debuglog("  M8+ detected: XISO support active");
		}
		else if (isIndpatched)
		{
			debuglog("");
			debuglog("[Bios]");
			debuglog("  Ind-Bios 5003 detected: XISO support active");
		}

		if (isM8Unpatched || isIndUnpatched)
		{
			if (g_cfg.UI_Enabled)
			{
				ShowGeneralScreen("Patching kernel...");
				Sleep(debuggingDelay);
			}
			std::ofstream PatcherXBEFile(PATCHER_FILE, std::ios::binary);
			PatcherXBEFile.write(reinterpret_cast<const char*>(kernel_patcher_header), kernel_patcher_header_size);
			char zeros[PATCHER_ZERO_PAD] = {0};
			PatcherXBEFile.write(zeros, PATCHER_ZERO_PAD);
			PatcherXBEFile.write(reinterpret_cast<const char*>(kernel_patcher_code), kernel_patcher_code_size);
			PatcherXBEFile.close();
			debuglog("");
			debuglog("[Kernel]");
			debuglog("  Patching kernel...");
			XLaunchXBE(PATCHER_FILE);
		}

		remove(PATCHER_FILE);
	}

	// Dismount virtual disc drive (Cerbios, M8+, IndBios & NKPatcher).
	if (!g_cfg.VirtualDrive_DismountISOOnIGR)
	{
		debuglog("");
		debuglog("[Virtual CDRom]");
		HANDLE h = NULL;
		NTSTATUS nt_status;
		ANSI_STRING dev_name;

		// Cerbios
		XMount("VD:", "\\Device\\Virtual0\\Image1");
		if (file_exist("VD:\\default.xbe"))
		{
			debuglog("  Cerbios ISO dismounted");
			ShowGeneralScreen("Dismounting Cerbios virtual ISO...");
			XUnmount("VD:");
			RtlInitAnsiString(&dev_name, "\\Device\\Virtual0\\Image0");
			IOCTL_VIRTUAL_CDROM = IOCTL_VIRTUAL_DETACH;
		}
		else
		{
			// NKPatcher (Softmods, M8+ and Ind-Bios 5003)
			XUnmount("VD:");
			XMount("VD:", "\\Device\\CdRom1");
			if (file_exist("VD:\\default.xbe"))
			{
				debuglog("  NKPatcher ISO dismounted");
				ShowGeneralScreen("Dismounting NKPatcher virtual ISO...");
				XUnmount("VD:");
				RtlInitAnsiString(&dev_name, "\\Device\\CdRom1");
				IOCTL_VIRTUAL_CDROM = IOCTL_VIRTUAL_CDROM_DETACH;
			}
			else
			{
				XUnmount("VD:");
				debuglog("  No virtual ISO mounted");
				goto skip_dismount;
			}
		}

		OBJECT_ATTRIBUTES obj_attr;
		obj_attr.RootDirectory = NULL;
		obj_attr.ObjectName = &dev_name;
		obj_attr.Attributes = OBJ_CASE_INSENSITIVE;

		IO_STATUS_BLOCK io_status;
		nt_status = NtOpenFile(&h, GENERIC_READ | SYNCHRONIZE, &obj_attr, &io_status, FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT);

		if (NT_SUCCESS(nt_status))
		{
			nt_status = NtDeviceIoControlFile(h, NULL, NULL, NULL, &io_status, IOCTL_VIRTUAL_CDROM, NULL, 0, NULL, 0);
			NtClose(h);
		}

		if (g_cfg.UI_Enabled)
			Sleep(debuggingDelay);
		skip_dismount:;
	}

	// This always gets checked first
	debuglog("");
	debuglog("[Setup and Updates]");
	debuglog("  Prep dashboard");
	try_launch("Prep dashboard", "E:\\Prep\\Default.xbe");

	// Rocky5 Softmod only
	if (softmodded)
	{
		debuglog("");
		debuglog("  Quick Update/Upgrade dashboard");
		try_launch("Quick Update dashboard", "E:\\Quick Update\\Default.xbe");
		try_launch("Quick Upgrade dashboard", "E:\\Quick Upgrade\\Default.xbe");

		debuglog("");
		debuglog("[Integrity Check]");
		debuglog("  ShadowC Partition");
		try_launch_error("ShadowC rescue dashboard", "C:\\nkpatcher\\rescuedash\\loader.xbe");
	}

	if (file_exist(ES_IGR))
	{
		debuglog("  Relaunch XBMC-Emustation");
		try_launch("XBMC-Emustation return to rom-list", ES_IGR);
	}

	// Check for button presses
	ShowGeneralScreen("Hold a button to select dashboard...");
	Sleep(g_cfg.UI_Enabled ? 1000 : 300);

	BOOL buttonFired = FALSE;
	int timer = 0;
	while (timer++ <= g_cfg.UI_ButtonDelay)
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

		if (!buttonFired && m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_Y] && (m_DefaultGamepad.wButtons & XINPUT_GAMEPAD_START))
		{
			buttonFired = TRUE;
			debuglog("");
			debuglog("[Button Window]");
			debuglog("  Force: Rescue dashboard locations");
			if (g_cfg.Recovery_Path[0])
				try_launch_error("Custom Recovery", g_cfg.Recovery_Path);
			try_launch_error("Rescue dashboard (TDATA)", "E:\\TDATA\\Rescuedash\\Default.xbe");
			try_launch_error("Rescue dashboard (UDATA)", "E:\\UDATA\\Rescuedash\\Default.xbe");

			// Rocky5 Softmod only
			if (softmodded)
				try_launch_error("ShadowC rescue dashboard", "R:\\NKPatcher\\rescuedash\\loader.xbe");
		}
		else if (!buttonFired && buttons_held_count() == 1)
		{
			buttonFired = TRUE;
			debuglog("");
			debuglog("[Button Window]");
			if (m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_A] && g_cfg.Btn_A[0])
			{
				debuglog("  A Button dashboard");
				try_launch_btn("A Button", g_cfg.Btn_A);
			}
			if (m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_B] && g_cfg.Btn_B[0])
			{
				debuglog("  B Button dashboard");
				try_launch_btn("B Button", g_cfg.Btn_B);
			}
			if (m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_X] && g_cfg.Btn_X[0])
			{
				debuglog("  X Button dashboard");
				try_launch_btn("X Button", g_cfg.Btn_X);
			}
			if (m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_Y] && g_cfg.Btn_Y[0])
			{
				debuglog("  Y Button dashboard");
				try_launch_btn("Y Button", g_cfg.Btn_Y);
			}
			if ((m_DefaultGamepad.wButtons & XINPUT_GAMEPAD_START) && g_cfg.Btn_Start[0])
			{
				debuglog("  Start Button dashboard");
				try_launch_btn("Start Button", g_cfg.Btn_Start);
			}
			if ((m_DefaultGamepad.wButtons & XINPUT_GAMEPAD_BACK) && g_cfg.Btn_Back[0])
			{
				debuglog("  Back Button dashboard");
				try_launch_btn("Back Button", g_cfg.Btn_Back);
			}
			if (m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_BLACK] && g_cfg.Btn_Black[0])
			{
				debuglog("  Black Button dashboard");
				try_launch_btn("Black Button", g_cfg.Btn_Black);
			}
			if (m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_WHITE] && g_cfg.Btn_White[0])
			{
				debuglog("  White Button dashboard");
				try_launch_btn("White Button", g_cfg.Btn_White);
			}
			debuglog("  No mapped path found or dashboard not found");
		}
		Sleep(1);
	}

	if (g_cfg.UI_Enabled)
	{
		ShowGeneralScreen("Checking dashboard paths...");
		Sleep(debuggingDelay);
	}

	if (g_cfg.Dashboard_Path[0])
	{
		debuglog("");
		debuglog("[Override Dashboard]");
		try_launch("Dashboard", g_cfg.Dashboard_Path);
	}

	debuglog("");
	debuglog("[Locations]");
	try_launch("E:\\XBMC-Emustation", "E:\\XBMC-Emustation\\Default.xbe");
	try_launch("E:\\XBMC4Gamers", "E:\\XBMC4Gamers\\Default.xbe");
	try_launch("E:\\XBMC4Xbox", "E:\\XBMC4Xbox\\Default.xbe");
	try_launch("E:\\Dashboard", "E:\\dashboard\\Default.xbe");
	try_launch("E:\\Dash", "E:\\Dash\\Default.xbe");
	try_launch("E:\\XBMC", "E:\\XBMC\\Default.xbe");
	try_launch("E:\\Default.xbe", "E:\\Default.xbe");
	try_launch("E:\\XBMC.xbe", "E:\\XBMC.xbe");
	try_launch("E:\\Evoxdash.xbe", "E:\\Evoxdash.xbe");


	try_launch("C:\\XBMC-Emustation", "C:\\XBMC-Emustation\\Default.xbe");
	try_launch("C:\\XBMC4Gamers", "C:\\XBMC4Gamers\\Default.xbe");
	try_launch("C:\\XBMC4Xbox", "C:\\XBMC4Xbox\\Default.xbe");
	try_launch("C:\\Dashboard", "C:\\dashboard\\Default.xbe");
	try_launch("C:\\Dash", "C:\\Dash\\Default.xbe");
	// Skip C:\Evoxdash.xbe if we're running from it to prevent a loop
	if (_stricmp(xbeName, "evoxdash.xbe") != 0)
		try_launch("C:\\Evoxdash.xbe", "C:\\Evoxdash.xbe");

	try_launch("F:\\XBMC-Emustation", "F:\\XBMC-Emustation\\Default.xbe");
	try_launch("F:\\XBMC4Gamers", "F:\\XBMC4Gamers\\Default.xbe");
	try_launch("F:\\XBMC4Xbox", "F:\\XBMC4Xbox\\Default.xbe");
	try_launch("F:\\XBMC", "F:\\XBMC\\Default.xbe");
	try_launch("F:\\Dashboard", "F:\\dashboard\\Default.xbe");
	try_launch("F:\\Dash", "F:\\Dash\\Default.xbe");

	// Last resort: rescue dashboards
	debuglog("");
	debuglog("[Rescue]");
	if (g_cfg.Recovery_Path[0])
		try_launch_error("Custom Recovery", g_cfg.Recovery_Path);
	try_launch_error("Rescue dashboard (TDATA)", "E:\\TDATA\\Rescuedash\\Default.xbe");
	try_launch_error("Rescue dashboard (UDATA)", "E:\\UDATA\\Rescuedash\\Default.xbe");

	// Rocky5 Softmod only
	if (softmodded)
		try_launch_error("ShadowC rescue dashboard", "R:\\NKPatcher\\rescuedash\\loader.xbe");

	// If you're here show UI and count down
	debuglog("All failed. :( Insert a disc and load from there");
	if (!g_cfg.UI_Enabled)
		InitD3D();
	ToWide("Dashloader v" BUILD_VERSION, g_wszHeader, 64);
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