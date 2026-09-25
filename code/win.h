/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2025 Electronic Arts Inc.
 * Copyright 2026 OpenTS contributors
 *
 * Contains material derived from Electronic Arts source code.
 * Modified by OpenTS contributors, 2026.
 * EA's GPLv3 Section 7 additional terms and supplemental warranty
 * disclaimers apply; see LICENSE.md.
 ******************************************************************************/

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwlib/win.h                                  $*
 *                                                                                             *
 *                      $Author:: Ian_l                                                       $*
 *                                                                                             *
 *                     $Modtime:: 10/16/01 2:42p                                              $*
 *                                                                                             *
 *                    $Revision:: 11                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#pragma once

#include <cstdint>

// Only the Windows files see the SDK; everything else is built without it.
#if defined(_WIN32)

// this define should also be in the DSP just in case someone includes windows stuff directly
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <mmsystem.h>
#include <winnt.h>
#include <winuser.h>

extern int			ShowCommand;
extern HINSTANCE	ProgramInstance;
extern HWND			MainWindow;

#else	// _WIN32

// Portable stand-ins for the Windows spellings that code not yet made platform-agnostic
// still names directly. A packed color or a message parameter is plain data everywhere;
// a handle is opaque and unused off Windows, so any real GDI or window-message call stays
// behind its own `#if defined(_WIN32)`.
typedef unsigned int COLORREF;
#define RGB(r, g, b) ((COLORREF)(((unsigned char)(r)) | (((unsigned char)(g)) << 8) | (((unsigned char)(b)) << 16)))
#define GetRValue(color) ((unsigned char)(color))
#define GetGValue(color) ((unsigned char)((color) >> 8))
#define GetBValue(color) ((unsigned char)((color) >> 16))

typedef unsigned int UINT;
typedef uintptr_t WPARAM;
typedef intptr_t LPARAM;
typedef intptr_t LRESULT;

// UIShellClass borrows Win32's window-message vocabulary as its own input-event protocol
// on every platform; only a real GDI or window handle stays behind `#if defined(_WIN32)`.
#define WM_KEYDOWN			0x0100
#define WM_KEYUP			0x0101
#define WM_CHAR				0x0102
#define WM_ACTIVATEAPP		0x001C
#define WM_CANCELMODE		0x001F
#define WM_SETCURSOR		0x0020
#define WM_INPUTLANGCHANGE	0x0051
#define WM_MOUSEMOVE		0x0200
#define WM_LBUTTONDOWN		0x0201
#define WM_LBUTTONUP		0x0202
#define WM_LBUTTONDBLCLK	0x0203
#define WM_RBUTTONDOWN		0x0204
#define WM_RBUTTONUP		0x0205
#define WM_RBUTTONDBLCLK	0x0206
#define WM_MBUTTONDOWN		0x0207
#define WM_MBUTTONUP		0x0208
#define WM_MBUTTONDBLCLK	0x0209
#define WM_MOUSEWHEEL		0x020A
#define WM_XBUTTONDOWN		0x020B
#define WM_XBUTTONUP		0x020C
#define WM_XBUTTONDBLCLK	0x020D
#define WM_MOUSEHWHEEL		0x020E
#define WM_CAPTURECHANGED	0x0215

#define VK_LBUTTON		0x01
#define VK_RBUTTON		0x02
#define VK_MBUTTON		0x04
#define VK_XBUTTON1		0x05
#define VK_XBUTTON2		0x06
#define VK_BACK			0x08
#define VK_TAB			0x09
#define VK_CLEAR		0x0C
#define VK_RETURN		0x0D
#define VK_SHIFT		0x10
#define VK_CONTROL		0x11
#define VK_MENU			0x12
#define VK_PAUSE		0x13
#define VK_CAPITAL		0x14
#define VK_ESCAPE		0x1B
#define VK_SPACE		0x20
#define VK_PRIOR		0x21
#define VK_NEXT			0x22
#define VK_END			0x23
#define VK_HOME			0x24
#define VK_LEFT			0x25
#define VK_UP			0x26
#define VK_RIGHT		0x27
#define VK_DOWN			0x28
#define VK_SNAPSHOT		0x2C
#define VK_INSERT		0x2D
#define VK_DELETE		0x2E
#define VK_LWIN			0x5B
#define VK_RWIN			0x5C
#define VK_APPS			0x5D
#define VK_NUMPAD0		0x60
#define VK_NUMPAD1		0x61
#define VK_NUMPAD2		0x62
#define VK_NUMPAD3		0x63
#define VK_NUMPAD4		0x64
#define VK_NUMPAD5		0x65
#define VK_NUMPAD6		0x66
#define VK_NUMPAD7		0x67
#define VK_NUMPAD8		0x68
#define VK_NUMPAD9		0x69
#define VK_MULTIPLY		0x6A
#define VK_ADD			0x6B
#define VK_SEPARATOR	0x6C
#define VK_SUBTRACT		0x6D
#define VK_DECIMAL		0x6E
#define VK_DIVIDE		0x6F
#define VK_F1			0x70
#define VK_F2			0x71
#define VK_F3			0x72
#define VK_F4			0x73
#define VK_F5			0x74
#define VK_F6			0x75
#define VK_F7			0x76
#define VK_F8			0x77
#define VK_F9			0x78
#define VK_F10			0x79
#define VK_F11			0x7A
#define VK_F12			0x7B
#define VK_NUMLOCK		0x90
#define VK_SCROLL		0x91
#define VK_LSHIFT		0xA0
#define VK_RSHIFT		0xA1
#define VK_LCONTROL		0xA2
#define VK_RCONTROL		0xA3
#define VK_LMENU		0xA4
#define VK_RMENU		0xA5
#define VK_OEM_1		0xBA
#define VK_OEM_PLUS		0xBB
#define VK_OEM_COMMA	0xBC
#define VK_OEM_MINUS	0xBD
#define VK_OEM_PERIOD	0xBE
#define VK_OEM_2		0xBF
#define VK_OEM_3		0xC0
#define VK_OEM_4		0xDB
#define VK_OEM_5		0xDC
#define VK_OEM_6		0xDD
#define VK_OEM_7		0xDE
#define VK_OEM_8		0xDF
#define VK_OEM_102		0xE2

#define MK_LBUTTON		0x0001

#define XBUTTON1		0x0001
#define XBUTTON2		0x0002

#define WHEEL_DELTA		120

#define CP_UTF8			65001

#define LOWORD(value) ((unsigned short)((std::uintptr_t)(value) & 0xffff))
#define HIWORD(value) ((unsigned short)(((std::uintptr_t)(value) >> 16) & 0xffff))
#define MAKELPARAM(low, high) ((LPARAM)(((unsigned short)(low)) | (((unsigned long)(unsigned short)(high)) << 16)))
#define MAKEWPARAM(low, high) ((WPARAM)(((unsigned short)(low)) | (((unsigned long)(unsigned short)(high)) << 16)))
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#define GET_XBUTTON_WPARAM(wp) (HIWORD(wp))

typedef int BOOL;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define MAX_PATH 260

typedef void * HANDLE;
typedef void * HWND;
typedef void * HDC;
typedef void * HBITMAP;
typedef void * HGDIOBJ;
typedef void * HFONT;
typedef void * HGLOBAL;
typedef void * HINSTANCE;

// The standard dialog command IDs, unrelated to any GDI or window handle.
#define IDOK		1
#define IDCANCEL	2
#define IDABORT		3
#define IDRETRY		4
#define IDIGNORE	5
#define IDYES		6
#define IDNO		7
#define IDCLOSE		8

#endif	// _WIN32

extern bool			GameInFocus;
