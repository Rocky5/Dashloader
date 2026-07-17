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

#ifndef DASHLOADER_FTP_H
#define DASHLOADER_FTP_H

#include "ftp.h"

static char s_ftpIP[20];

static int Percent(unsigned long done, unsigned long total)
{
	unsigned long d = done, t = total;
	if (t == 0) return 0;
	while (t > 40000000UL) { d >>= 1; t >>= 1; }
	if (t == 0) t = 1;
	if (d > t) d = t;
	return (int)((d * 100UL) / t);
}

static void IToA(int v, char* out)
{
	char t[12];
	int n = 0, i = 0, neg = 0;
	if (v < 0) { neg = 1; v = -v; }
	if (v == 0) t[n++] = '0';
	while (v) { t[n++] = (char)('0' + v % 10); v /= 10; }
	if (neg) out[i++] = '-';
	while (n) out[i++] = t[--n];
	out[i] = 0;
}

static void OutputSize(unsigned long bytes, char* out)
{
	if (bytes >= 1073741824UL)
	{
		unsigned long gb = bytes / 1073741824UL;
		unsigned long frac = (bytes % 1073741824UL) / 10737418UL;
		_snprintf(out, 16, "%lu.%02lu GB", gb, frac);
	}
	else if (bytes >= 1048576UL)
	{
		unsigned long mb = bytes / 1048576UL;
		unsigned long frac = (bytes % 1048576UL) / 10485UL;
		_snprintf(out, 16, "%lu.%02lu MB", mb, frac);
	}
	else if (bytes >= 1024UL)
	{
		_snprintf(out, 16, "%lu KB", bytes / 1024UL);
	}
	else
	{
		_snprintf(out, 16, "%lu B", bytes);
	}
}

static void FTP_Start()
{
	debuglog("");
	debuglog("[FTP Mode]");

	if (!(XNetGetEthernetLinkStatus() & XNET_ETHERNET_LINK_ACTIVE))
	{
		debuglog("  No network link - FTP not started");
		return;
	}

	XNetStartupParams xnsp;
	WSADATA WsaData;
	memset(&xnsp, 0, sizeof(xnsp));
	xnsp.cfgSizeOfStruct = sizeof(XNetStartupParams);
	xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
	xnsp.cfgSockMaxSockets = 64;

	if (XNetStartup(&xnsp) != 0)
	{
		debuglog("  XNetStartup failed");
		return;
	}
	if (WSAStartup(MAKEWORD(2, 2), &WsaData) != NO_ERROR)
	{
		debuglog("  WSAStartup failed");
		return;
	}

	XNADDR xna;
	memset(&xna, 0, sizeof(xna));
	DWORD deadline = GetTickCount() + 5000;
	while (GetTickCount() < deadline)
	{
		if (XNetGetTitleXnAddr(&xna) != XNET_GET_XNADDR_PENDING)
			break;
		Sleep(100);
	}

	Ftp_Init();

	XNetInAddrToString(xna.ina, s_ftpIP, sizeof(s_ftpIP));

	FtpServ_Start(s_ftpIP, 1);

	debuglog("  IP: %s", s_ftpIP);
	DWORD ls = XNetGetEthernetLinkStatus();
	debuglog("  Link: %s", (ls & XNET_ETHERNET_LINK_ACTIVE) ? "Active" : "No link");
}

static void FTP_Stop()
{
	FtpServ_Stop();
	WSACleanup();
	XNetCleanup();
	s_ftpIP[0] = '\0';
	debuglog("[FTP] Stopped");
}

static void RunFTP(int useErrorScreen, int softmodded = false)
{
	XUnmount("R:");
	XUnmount("D:");
	XMount("D:", "\\Device\\CdRom0");
	XMount("X:", "\\Device\\Harddisk0\\Partition3");
	XMount("Y:", "\\Device\\Harddisk0\\Partition4");
	XMount("G:", "\\Device\\Harddisk0\\Partition7");

	static const char ftpStatus[] = "FTP active\n";
	static const char rebootLabel[] = "\n\nPress A+B reboot";
	static const char sepLabel[] = " / ";
	static const char uploadingLabel[] = "Uploading:\n";
	static const char downloadingLabel[] = "Downloading:\n";
	static const char uploadDoneLabel[] = "Upload complete:\n";
	static const char downloadDoneLabel[] = "Download complete:\n";

	char msg[256];
	char progress[64];

	FTP_Start();

	unsigned long xferDone = 0;
	unsigned long xferTotal = 0;
	unsigned long xferSpeed = 0;
	DWORD xferStart = 0;
	DWORD delayElapsed = 0;
	FtpXfer xferType = XFER_NONE;
	int lastStatus = 0;
	int allowReboot = 1;

	if (s_ftpIP[0])
		_snprintf(msg, sizeof(msg), "%s%s\nUser: xbox Pass: xbox%s", ftpStatus, s_ftpIP, rebootLabel);
	else
		_snprintf(msg, sizeof(msg), "FTP: No network link.%s", rebootLabel);

	msg[sizeof(msg) - 1] = '\0';

	if (useErrorScreen)
		ShowErrorScreen(msg);
	else
		ShowButtonPressScreen(msg);

	while (1)
	{
		FtpServ_Tick();

		unsigned long done = 0;
		unsigned long total = 0;
		int status = Ftp_Status();
		const char* screen = NULL;

		Ftp_Progress(&done, &total);

		if (status == FTP_TRANSFER)
		{
			xferDone = done;
			xferTotal = total;
			xferType = g_ftp.xferType;
		}

		if (status == FTP_TRANSFER && done > 0)
		{
			allowReboot = 0;

			if (xferStart == 0)
				xferStart = GetTickCount();

			DWORD delay = GetTickCount();
			DWORD elapsed = GetTickCount() - xferStart;
			if (delay - delayElapsed >= 1000)
			{
				if (elapsed >= 1000)
					xferSpeed = done / (elapsed / 1000);
				else if (elapsed > 0)
					xferSpeed = done * 1000UL / elapsed;
				delayElapsed = delay;

				char szDone[16];
				char szSpeed[16];
				OutputSize(done, szDone);
				OutputSize(xferSpeed, szSpeed);

				if (total > 0)
				{
					char percent[12];
					char szTotal[16];
					IToA(Percent(done, total), percent);
					OutputSize(total, szTotal);
					const char* label = (g_ftp.xferType == XFER_STOR) ? uploadingLabel : downloadingLabel;
					_snprintf(progress, sizeof(progress), "%s%s%% %s%s%s (%s/s)", label, percent, szDone, sepLabel, szTotal, szSpeed);
				}
				else
					_snprintf(progress, sizeof(progress), "%s%s (%s/s)", uploadingLabel, szDone, szSpeed);

				screen = progress;
			}
		}
		else if (status == FTP_CONNECTED && lastStatus == FTP_TRANSFER && xferDone > 0)
		{
			xferStart = 0;
			xferSpeed = 0;
			char szDone[16];
			OutputSize(xferDone, szDone);
			const char* doneLabel = (xferType == XFER_STOR) ? uploadDoneLabel : downloadDoneLabel;
			if (xferTotal > 0)
			{
				char szTotal[16];
				OutputSize(xferTotal, szTotal);
				_snprintf(progress, sizeof(progress), "%s%s%s%s%s", doneLabel, szDone, sepLabel, szTotal, rebootLabel);
			}
			else
				_snprintf(progress, sizeof(progress), "%s%s%s", doneLabel, szDone, rebootLabel);

			screen = progress;
			allowReboot = 1;
		}

		if (screen)
		{
			if (useErrorScreen)
				ShowErrorScreen(screen);
			else
				ShowButtonPressScreen(screen);
		}

		lastStatus = status;

		XBInput_GetInput(m_Gamepad);

		ZeroMemory(&m_DefaultGamepad, sizeof(m_DefaultGamepad));

		for (DWORD _i = 0; _i < 4; _i++)
		{
			if (m_Gamepad[_i].hDevice)
			{
				m_DefaultGamepad.wButtons |= m_Gamepad[_i].wButtons;

				for (DWORD _b = 0; _b < 8; _b++)
					m_DefaultGamepad.bAnalogButtons[_b] |= m_Gamepad[_i].bAnalogButtons[_b];
			}
		}

		if (allowReboot && m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_A] && m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_B])
			break;

		if (softmodded && allowReboot && m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_X] && m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_B])
		{
			XMount("R:", "\\Device\\Harddisk0\\Partition14");
			_snprintf(msg, sizeof(msg), "%s%s\nUser: xbox Pass: xbox\nUnsecure Mode%s", ftpStatus, s_ftpIP, rebootLabel);
			ShowErrorScreen(msg);
		}
	}

	FTP_Stop();
}

#endif