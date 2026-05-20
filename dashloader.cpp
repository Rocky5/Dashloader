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
#include <iostream>
#include "kernelpatcher.h"
#include "external.h"
#include "dashloader_ui.h"

#define BUILD_VERSION "2.0.0"

#define INI_FILE         "D:\\Dashloader.ini"
#define DISC_XBE         "D:\\default.xbe"
#define ES_IGR           "E:\\CACHE\\LocalCache20.bin"
#define Ind_Checker_File "E:\\CACHE\\LocalCache30.bin"
#define Patcher_File     "E:\\CACHE\\LocalCache40.bin"

#define Patched_M8       ((char*)0x8002691E)
#define Patched_Ind      ((char*)0x8002B4B7)
#define M8_CHECK         ((char*)0x8002690E)

DASHLOADER_CONFIG g_cfg;

char xbeName[64] = "";
int legacy = 0;
int patched = 0;
int softmodded = 0;

void __cdecl main()
{
	XMountRunningXBEDir();
	XMount("C:", "\\Device\\Harddisk0\\Partition2");
	XMount("E:", "\\Device\\Harddisk0\\Partition1");
	XMount("F:", "\\Device\\Harddisk0\\Partition6");
	XMount("R:", "\\Device\\Harddisk0\\Partition14");
	CreateDirectory("E:\\CACHE", NULL);
	
	// Check if we are running my softmod. (aka Rocky5 softmod)
	softmodded = file_exist("R:\\nkpatcher\\default.xbe");

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
		SetStatus("Starting...");
	}

	XInitDevices(0, NULL);
	if (FAILED(XBInput_CreateGamepads(&m_Gamepad)))
		debuglog("ERROR: Cant create gamepad");

	// Ind-Bios 5003 / M8+ virtual disc loader patch
	if (!g_cfg.VirtualDrive_ISOKernelPatch)
	{
		// Ind-Bios = 5003
		// M8+ = 5838 + ÿ (otherwise v1.6 kernel will be picked up)
		if (kernelBuild == 5838 && *M8_CHECK == 'ÿ')
		{
			debuglog("");
			debuglog("[Bios]");
			debuglog("  M8+ detected: XISO support active");
			patched = 1;
		}
		else if (kernelBuild == 5838 && *Patched_M8 == '™')
		{
			debuglog("");
			debuglog("[Bios]");
			debuglog("  M8+ detected: already patched");
			legacy = 0;
		}
		else if (kernelBuild == 5003)
		{
			debuglog("");
			debuglog("[Bios]");
			debuglog("  Ind-Bios 5003 detected: XISO support active");
			if (*Patched_Ind != '™')
				patched = 1;
			legacy = 1;
		}

		if (patched)
		{
			if (!file_exist(Patcher_File))
			{
				debuglog("");
				debuglog("[Kernel]");
				debuglog("  Patching kernel...");
				std::ofstream PatcherXBEFile(Patcher_File, std::ios::binary);
				PatcherXBEFile.write(reinterpret_cast<const char*>(kernel_patcher_header), kernel_patcher_header_size);
				char zeros[PATCHER_ZERO_PAD] = {0};
				PatcherXBEFile.write(zeros, PATCHER_ZERO_PAD);
				PatcherXBEFile.write(reinterpret_cast<const char*>(kernel_patcher_code), kernel_patcher_code_size);
				PatcherXBEFile.close();
				XLaunchXBE(Patcher_File);
			}
			remove(Patcher_File);
		}

		// Ind-Bios needs a manual disc load so handle IGR exit
		// Cerbios attacher already does this, but I have no way of knowing what xbe launched the ISO
		if (legacy)
		{
			XUnmount("D:");
			XMount("D:", "\\Device\\CdRom0");
			if (!file_exist(Ind_Checker_File) && file_exist(DISC_XBE))
			{
				// Creating blank file so we don't keep reloading the game.
				// This wouldn't be required if Ind-Bios didn't need a manual disc load.
				std::ofstream blankFile(Ind_Checker_File, std::ios::binary);
				blankFile.close();

				// This is so we can exit the game when we IGR. If we don't it does what M8+ does.
				// If using cerbios new attacher xbe you will need to IGR twice.
				debuglog("  Launching D:\\default.xbe for clean IGR exit");
				SetStatus("Ind-Bios: launching D:\\default.xbe for clean IGR exit");
				XLaunchXBE(DISC_XBE);
			}
			remove(Ind_Checker_File);
		}
	}

	// Dismount virtual disc drive (Cerbios, M8+, IndBios & NKPatcher).
	if (!g_cfg.VirtualDrive_DismountISOOnIGR)
	{
		HANDLE h = NULL;
		NTSTATUS nt_status;
		ANSI_STRING dev_name;
		// this is for softmods, NKPatcher mounts to Cdrom1
		XMount("VD:", "\\Device\\Cdrom1");
		if (file_exist("VD:\\default.xbe") || legacy)
		{
			legacy = 1;
			RtlInitAnsiString(&dev_name, "\\Device\\CdRom1");
		}
		else
			RtlInitAnsiString(&dev_name, "\\Device\\Virtual0\\Image0");

		unsigned long IOCTL_VIRTUAL_CDROM = legacy ? IOCTL_VIRTUAL_CDROM_DETACH : IOCTL_VIRTUAL_DETACH;
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
	}

	SetStatus("Hold a button to select dashboard...");
	if (g_cfg.UI_Enabled)
		Sleep(1000);
	else
		Sleep(300);

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
				try_launch_btn("Custom Recovery", g_cfg.Recovery_Path);
			try_launch("Rescue dashboard (TDATA)", "E:\\TDATA\\Rescuedash\\Default.xbe");
			try_launch("Rescue dashboard (UDATA)", "E:\\UDATA\\Rescuedash\\Default.xbe");

			// Softmod only
			if (softmodded)
				try_launch("ShadowC rescue dashboard", "R:\\NKPatcher\\rescuedash\\loader.xbe");
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

	debuglog("");
	debuglog("[Dashboard]");
	SetStatus("Checking dashboard paths...");
	debuglog("  Prep dashboard");
	try_launch("Prep dashboard", "E:\\Prep\\Default.xbe");

	// Softmod only
	if (softmodded)
	{
		debuglog("  Quick Update/Upgrade dashboard");
		try_launch("Quick Update dashboard", "E:\\Quick Update\\Default.xbe");
		try_launch("Quick Upgrade dashboard", "E:\\Quick Upgrade\\Default.xbe");

		debuglog("  ShadowC Partition Integrity");
		try_launch("ShadowC partition integrity", "C:\\nkpatcher\\rescuedash\\loader.xbe");
	}

	if (file_exist(ES_IGR))
	{
		debuglog("  Relaunch XBMC-Emustation");
		try_launch("XBMC-Emustation return to rom-list", ES_IGR);
	}

	debuglog("  Custom Dashboard");
	if (g_cfg.Dashboard_Path[0])
		try_launch_btn("Custom dashboard", g_cfg.Dashboard_Path);

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
		try_launch_btn("Custom Recovery", g_cfg.Recovery_Path);
	try_launch("Rescue dashboard (TDATA)", "E:\\TDATA\\Rescuedash\\Default.xbe");
	try_launch("Rescue dashboard (UDATA)", "E:\\UDATA\\Rescuedash\\Default.xbe");

	// Softmod only
	if (softmodded)
		try_launch("ShadowC rescue dashboard", "R:\\NKPatcher\\rescuedash\\loader.xbe");

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
		SetError(msg);
		Sleep(1000);
		failedTimer--;
	}
	fail_reboot();
}