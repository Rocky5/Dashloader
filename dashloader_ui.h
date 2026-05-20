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

#ifndef DASHLOADER_UI_H
#define DASHLOADER_UI_H

#include <XBFont.h>
#include <XBUtil.h>
#include <fstream>
#include "font.h"
#include "dashloader.h"
#include "xbinput.h"

// Gamepad members
XBGAMEPAD* m_Gamepad;
XBGAMEPAD  m_DefaultGamepad;

#define COLOR_BACKGROUND 0x00000000  //0xFF060d18
#define COLOR_HEADER     0xFFFFFFFF
#define COLOR_STATUS     0XFF2CC1FF
#define COLOR_ERROR      0xFFFF2C2C
#define COLOR_BUTTON     0xFFf8ff2c

extern DASHLOADER_CONFIG g_cfg;

static FILE* logfile = NULL;
static LPDIRECT3D8 g_pD3D = NULL;
static LPDIRECT3DDEVICE8 g_pDevice = NULL;
static CXBFont g_Font;
static BOOL g_bD3DReady = FALSE;
static WCHAR g_wszHeader[64];
static WCHAR g_wszStatus[512];


int file_exist(char* name)
{
	struct stat buffer;
	return (stat(name, &buffer) == 0);
}


void debuglog(const char* format, ...)
{
	if (!g_cfg.Log_Enabled)
		return;

	char buffer[1024];
	va_list va;
	va_start(va, format);
	vsprintf(buffer, format, va);
	va_end(va);
	strcat(buffer, "\n");
	if (logfile)
	{
		fputs(buffer, logfile);
		fflush(logfile);
	}
}


static void ToWide(const char* src, WCHAR* dst, int maxlen)
{
	int i = 0;
	for (; src[i] && i < maxlen - 1; i++)
		dst[i] = (WCHAR)(unsigned char)src[i];
	dst[i] = L'\0';
}


extern "C" void CloseUI()
{
	if (!g_bD3DReady)
		return;

	g_Font.Destroy();

	if (g_pDevice)
	{
		g_pDevice->Release();
		g_pDevice = NULL;
	}
	if (g_pD3D)
	{
		g_pD3D->Release();
		g_pD3D = NULL;
	}

	g_bD3DReady = FALSE;
}


static void InitD3D()
{
	int i;
	std::ofstream writeFont("Z:\\font.xpr", std::ios::binary);
	for (i = 0; i < sizeof(font_data); i++)
		writeFont << font_data[i];
	writeFont.close();

	g_pD3D = Direct3DCreate8(D3D_SDK_VERSION);
	if (!g_pD3D)
		return;

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));

	DWORD dwVideoFlags = XGetVideoFlags();

	d3dpp.BackBufferWidth = 640;
	d3dpp.BackBufferHeight = 480;
	d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
	d3dpp.BackBufferCount = 1;
	d3dpp.EnableAutoDepthStencil = FALSE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	if (dwVideoFlags & XC_VIDEO_FLAGS_HDTV_720p)
	{
		d3dpp.BackBufferWidth = 1280;
		d3dpp.BackBufferHeight = 720;
		d3dpp.Flags = D3DPRESENTFLAG_PROGRESSIVE | D3DPRESENTFLAG_WIDESCREEN;
	}
	else if (dwVideoFlags & XC_VIDEO_FLAGS_HDTV_480p)
		d3dpp.Flags = D3DPRESENTFLAG_PROGRESSIVE;
	else if (XGetVideoStandard() == XC_VIDEO_STANDARD_PAL_I)
		d3dpp.Flags = D3DPRESENTFLAG_INTERLACED | D3DPRESENTFLAG_FIELD;
	else
		d3dpp.Flags = D3DPRESENTFLAG_INTERLACED;

	if (FAILED(g_pD3D->CreateDevice(0, D3DDEVTYPE_HAL, NULL,
		D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &g_pDevice)))
	{
		g_pD3D->Release();
		g_pD3D = NULL;
		return;
	}

	extern LPDIRECT3DDEVICE8 g_pd3dDevice;
	g_pd3dDevice = g_pDevice;

	XBUtil_SetMediaPath("Z:\\");

	if (FAILED(g_Font.Create("font.xpr", 0)))
	{
		debuglog("WARNING: font.xpr not found, no screen output");
		g_pDevice->Release();
		g_pD3D->Release();
		g_pDevice = NULL;
		g_pD3D = NULL;
		return;
	}

	g_bD3DReady = TRUE;
}


static void ShowScreen()
{
	if (!g_bD3DReady)
		return;

	g_pDevice->Clear(0, NULL, D3DCLEAR_TARGET, COLOR_BACKGROUND, 1.0f, 0);
	g_pDevice->BeginScene();
	g_Font.DrawText(40, 40, COLOR_HEADER, g_wszHeader, 0);
	g_Font.DrawText(40, 55, COLOR_STATUS, g_wszStatus, 0);
	g_pDevice->EndScene();
	g_pDevice->Present(NULL, NULL, NULL, NULL);
}


static void SetStatus(const char* msg)
{
	ToWide(msg, g_wszStatus, 512);
	ShowScreen();
}


static void SetButtonPress(const char* msg)
{
	if (!g_bD3DReady)
		return;

	ToWide(msg, g_wszStatus, 512);
	g_pDevice->Clear(0, NULL, D3DCLEAR_TARGET, COLOR_BACKGROUND, 1.0f, 0);
	g_pDevice->BeginScene();
	g_Font.DrawText(40, 40, COLOR_HEADER, g_wszHeader, 0);
	g_Font.DrawText(40, 55, COLOR_BUTTON, g_wszStatus, 0);
	g_pDevice->EndScene();
	g_pDevice->Present(NULL, NULL, NULL, NULL);
}


static void SetError(const char* msg)
{
	if (!g_bD3DReady)
		return;

	ToWide(msg, g_wszStatus, 512);
	g_pDevice->Clear(0, NULL, D3DCLEAR_TARGET, COLOR_BACKGROUND, 1.0f, 0);
	g_pDevice->BeginScene();
	g_Font.DrawText(40, 40, COLOR_HEADER, g_wszHeader, 0);
	g_Font.DrawText(40, 55, COLOR_ERROR, g_wszStatus, 0);
	g_pDevice->EndScene();
	g_pDevice->Present(NULL, NULL, NULL, NULL);
}


static void try_launch(const char* description, const char* path)
{
	char msg[MAX_PATH + 64];
	debuglog("  > %s", path);

	if (file_exist(const_cast<char*>(path)) && g_cfg.UI_Enabled)
	{
		sprintf(msg, "  Launching: %s", description);
		SetStatus(msg);
		Sleep(g_cfg.UI_LaunchDelay);
		CloseUI();
	}

	XLaunchXBE(path);
	debuglog("    > not found");
}

static void try_launch_btn(const char* description, const char* path)
{
	if (!path || path[0] == '\0')
		return;

	char msg[MAX_PATH + 64];
	debuglog("  > %s", path);

	if (file_exist(const_cast<char*>(path)) && g_cfg.UI_Enabled)
	{
		sprintf(msg, "  Launching: %s", description);
		SetButtonPress(msg);
		Sleep(g_cfg.UI_LaunchDelay);
		CloseUI();
	}

	XLaunchXBE(path);
	debuglog("    > not found");
}

static void fail_reboot()
{
	debuglog("All launch attempts failed. Rebooting.");
	if (logfile)
	{
		fflush(logfile);
		fclose(logfile);
		logfile = NULL;
	}
	XReboot();
}

static int buttons_held_count()
{
	int held = 0;

	if (m_DefaultGamepad.wButtons & XINPUT_GAMEPAD_START)
		held++;
	if (m_DefaultGamepad.wButtons & XINPUT_GAMEPAD_BACK)
		held++;

	for (DWORD b = 0; b < 8; b++)
	{
		if (m_DefaultGamepad.bAnalogButtons[b] > 0)
			held++;
	}

	return held;
}

#endif