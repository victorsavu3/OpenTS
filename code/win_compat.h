/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

// Defines only the Win32 names the inherited tree references directly; code/keyboard.h
// defines the VK_* table itself and does not depend on this header.

#include <cstddef>
#include <cstdint>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <thread>

using BYTE = std::uint8_t;
using PBYTE = std::uint8_t *;
using WORD = std::uint16_t;
using DWORD = std::uint32_t;
using ULONG = std::uint32_t;
using LONG = std::int32_t;
using SHORT = std::int16_t;
using __int64 = std::int64_t;
using UINT = unsigned int;
using BOOL = int;
using VOID = void;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define CALLBACK
#define WINAPI
#define _ReturnAddress() __builtin_return_address(0)

// SAL-style parameter direction markers; windows.h defines these as nothing too.
#define IN
#define OUT
#define OPTIONAL

// Opaque handles; nothing on this build dereferences one as a real Win32 object.
using HANDLE = void *;
using HINSTANCE = void *;
using HWND = void *;
using HDC = void *;
using HMENU = void *;
using HICON = void *;
using HCURSOR = void *;
using HKEY = void *;
using HBITMAP = void *;
using HFONT = void *;
using HMODULE = void *;
using HGLOBAL = void *;
using HGDIOBJ = void *;

using WPARAM = std::uintptr_t;
using LPARAM = std::intptr_t;
using LRESULT = std::intptr_t;
using DWORD_PTR = std::uintptr_t;
using INT_PTR = std::intptr_t;
using UINT_PTR = std::uintptr_t;
using LONG_PTR = std::intptr_t;

using LPVOID = void *;
using LPSTR = char *;
using LPCSTR = char const *;
using LPWSTR = wchar_t *;
using LPCWSTR = wchar_t const *;

struct POINT
{
	LONG x;
	LONG y;
};
using LPPOINT = POINT *;

struct SIZE
{
	LONG cx;
	LONG cy;
};

struct POINTS
{
	SHORT x;
	SHORT y;
};

struct RECT
{
	LONG left;
	LONG top;
	LONG right;
	LONG bottom;
};

struct MSG
{
	HWND hwnd;
	UINT message;
	WPARAM wParam;
	LPARAM lParam;
	DWORD time;
	POINT pt;
};

using COLORREF = DWORD;
#define RGB(r, g, b) ((COLORREF)(((BYTE)(r) | ((WORD)(BYTE)(g) << 8)) | (((DWORD)(BYTE)(b)) << 16)))

#define LOWORD(l) ((WORD)((std::uintptr_t)(l) & 0xffff))
#define HIWORD(l) ((WORD)(((std::uintptr_t)(l) >> 16) & 0xffff))
#define LOBYTE(w) ((BYTE)((std::uintptr_t)(w) & 0xff))
#define HIBYTE(w) ((BYTE)(((std::uintptr_t)(w) >> 8) & 0xff))
#define MAKEWORD(a, b) ((WORD)(((BYTE)(a)) | (((WORD)((BYTE)(b))) << 8)))
#define MAKELONG(a, b) ((LONG)(((WORD)(a)) | (((DWORD)((WORD)(b))) << 16)))
#define MAKELPARAM(a, b) ((LPARAM)MAKELONG(a, b))
#define MAKEWPARAM(a, b) ((WPARAM)MAKELONG(a, b))
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#define GET_WHEEL_DELTA_WPARAM(wParam) ((short)HIWORD(wParam))
#define GET_XBUTTON_WPARAM(wParam) (HIWORD(wParam))
#define MAKEPOINTS(l) (POINTS{(SHORT)LOWORD(l), (SHORT)HIWORD(l)})

// Mouse-button-state flags carried in a WM_MOUSEMOVE/WM_*BUTTON* message's wParam.
#define MK_LBUTTON   0x0001
#define MK_RBUTTON   0x0002
#define MK_SHIFT     0x0004
#define MK_CONTROL   0x0008
#define MK_MBUTTON   0x0010
#define MK_XBUTTON1  0x0020
#define MK_XBUTTON2  0x0040

#define XBUTTON1 0x0001
#define XBUTTON2 0x0002
#define WHEEL_DELTA 120

// Window message values match winuser.h, though nothing here crosses a real Windows queue.
#define WM_CREATE          0x0001
#define WM_DESTROY         0x0002
#define WM_MOVE            0x0003
#define WM_SIZE            0x0005
#define WM_SETFONT         0x0030
#define WM_PAINT           0x000F
#define WM_CLOSE           0x0010
#define WM_ERASEBKGND      0x0014
#define WM_SHOWWINDOW      0x0018
#define WM_ACTIVATEAPP     0x001C
#define WM_CANCELMODE      0x001F
#define WM_SETCURSOR       0x0020
#define WM_INPUTLANGCHANGE 0x0051
#define WM_HELP            0x0053
#define WM_CONTEXTMENU     0x007B
#define WM_DISPLAYCHANGE   0x007E
#define WM_KEYDOWN         0x0100
#define WM_KEYUP           0x0101
#define WM_CHAR            0x0102
#define WM_SYSKEYDOWN      0x0104
#define WM_SYSKEYUP        0x0105
#define WM_INITDIALOG      0x0110
#define WM_COMMAND         0x0111
#define WM_SYSCOMMAND      0x0112
#define WM_TIMER           0x0113
#define WM_MOUSEMOVE       0x0200
#define WM_LBUTTONDOWN     0x0201
#define WM_LBUTTONUP       0x0202
#define WM_LBUTTONDBLCLK   0x0203
#define WM_RBUTTONDOWN     0x0204
#define WM_RBUTTONUP       0x0205
#define WM_RBUTTONDBLCLK   0x0206
#define WM_MBUTTONDOWN     0x0207
#define WM_MBUTTONUP       0x0208
#define WM_MBUTTONDBLCLK   0x0209
#define WM_MOUSEWHEEL      0x020A
#define WM_XBUTTONDOWN     0x020B
#define WM_XBUTTONUP       0x020C
#define WM_XBUTTONDBLCLK   0x020D
#define WM_MOUSEHWHEEL     0x020E
#define WM_MOUSELAST       WM_MOUSEHWHEEL
#define WM_CAPTURECHANGED  0x0215
#define WM_MOVING          0x0216
#define WM_USER            0x0400
#define WM_APP             0x8000

// MessageBox flags.
#define MB_OK              0x00000000
#define MB_YESNO           0x00000004
#define MB_ICONSTOP        0x00000010
#define MB_ICONERROR       MB_ICONSTOP
#define MB_ICONQUESTION    0x00000020
#define MB_ICONEXCLAMATION 0x00000030
#define MB_ICONWARNING     MB_ICONEXCLAMATION
#define MB_SYSTEMMODAL     0x00001000
#define MB_SETFOREGROUND   0x00010000
#define MB_TOPMOST         0x00040000

// Window styles and show commands.
#define WS_OVERLAPPEDWINDOW 0x00CF0000
#define WS_POPUP            0x80000000
#define CS_HREDRAW          0x0002
#define CS_VREDRAW          0x0001
#define CS_DBLCLKS          0x0008
#define SW_NORMAL           1
#define SW_RESTORE          9
#define SW_MINIMIZE         6
#define GWL_STYLE           (-16)
#define GWL_EXSTYLE         (-20)
#define GWL_ID              (-12)
#define SC_CLOSE            0xF060
#define SC_SCREENSAVE       0xF140

// GetSystemMetrics indices.
#define SM_CXSCREEN       0
#define SM_CYSCREEN       1
#define SM_CXFULLSCREEN   16
#define SM_CYFULLSCREEN   17
#define SM_CXDRAG         68
#define SM_CYDRAG         69
#define SM_SWAPBUTTON     23

#define CF_UNICODETEXT 13

// Trackbar messages for winfix.h's Slider_* macros; nothing here creates a real control.
inline LRESULT SendMessage(HWND, UINT, WPARAM, LPARAM) { return(0); }
#define SNDMSG SendMessage
#define TBM_GETPOS       0x0400
#define TBM_GETRANGEMIN  0x0401
#define TBM_GETRANGEMAX  0x0402
#define TBM_GETTIC       0x0403
#define TBM_SETTIC       0x0404
#define TBM_SETPOS       0x0405
#define TBM_SETRANGE     0x0406
#define TBM_SETRANGEMIN  0x0407
#define TBM_SETRANGEMAX  0x0408
#define TBM_CLEARTICS    0x0409
#define TBM_SETSEL       0x040A
#define TBM_SETSELSTART  0x040B
#define TBM_SETSELEND    0x040C

#define IDOK     1
#define IDCANCEL 2
#define IDYES    6
#define IDNO     7

using ULONGLONG = std::uint64_t;
using LPCTSTR = char const *;

#define MAKEINTRESOURCEA(i) ((LPSTR)(std::uintptr_t)(WORD)(i))
#define MAKEINTRESOURCE MAKEINTRESOURCEA

// Standard cursor IDs, named the way LoadCursor(NULL, IDC_*) expects.
#define IDC_ARROW    MAKEINTRESOURCE(32512)
#define IDC_IBEAM    MAKEINTRESOURCE(32513)
#define IDC_WAIT     MAKEINTRESOURCE(32514)
#define IDC_CROSS    MAKEINTRESOURCE(32515)
#define IDC_SIZENWSE MAKEINTRESOURCE(32642)
#define IDC_SIZENESW MAKEINTRESOURCE(32643)
#define IDC_SIZEWE   MAKEINTRESOURCE(32644)
#define IDC_SIZENS   MAKEINTRESOURCE(32645)
#define IDC_SIZEALL  MAKEINTRESOURCE(32646)
#define IDC_NO       MAKEINTRESOURCE(32648)
#define IDC_HAND     MAKEINTRESOURCE(32649)

#define GetRValue(c) ((BYTE)(c))
#define GetGValue(c) ((BYTE)(((WORD)(c)) >> 8))
#define GetBValue(c) ((BYTE)((c) >> 16))

// Nothing on this build has a window-class-registered cursor, so this always reports none
// and callers fall back to loading one by ID.
#define GCLP_HCURSOR (-12)
inline LONG_PTR GetClassLongPtr(HWND, int) { return(0); }

// Implemented in sdlstub.cpp: maps an IDC_* id to an SDL system cursor and applies it.
HCURSOR LoadCursor(HINSTANCE, LPCTSTR id);
void SetCursor(HCURSOR cursor);

// Only ever called on a cursor Build_Cursor (wincursor.cpp) made with SDL_CreateColorCursor,
// never on one of the cached system cursors LoadCursor hands out.
void DestroyCursor(HCURSOR cursor);

#define CP_ACP 0
#define CP_UTF8 65001

// This build's window never distinguishes ANSI from Unicode, so it always answers as one.
inline BOOL IsWindowUnicode(HWND) { return(TRUE); }

// SDL always hands text input as UTF-8, so the process codepage is always CP_UTF8 here.
inline unsigned int GetACP(void) { return(CP_UTF8); }

// No Windows installation directory exists to look a system font up in.
inline unsigned int GetWindowsDirectoryA(char *, unsigned int) { return(0); }

struct ULARGE_INTEGER
{
	std::uint64_t QuadPart;
};

// GetDiskFreeSpaceEx/GetLastError are implemented in sdlstub.cpp.
BOOL GetDiskFreeSpaceEx(char const * path, ULARGE_INTEGER * free_available,
	ULARGE_INTEGER * total, ULARGE_INTEGER * total_free);
DWORD GetLastError(void);

#define MAKELANGID(primary, sub) (((WORD)(sub) << 10) | (WORD)(primary))
#define LANG_NEUTRAL     0x00
#define SUBLANG_DEFAULT  0x01

struct MSGBOXPARAMS
{
	UINT cbSize;
	HWND hwndOwner;
	HINSTANCE hInstance;
	char const * lpszText;
	char const * lpszCaption;
	DWORD dwStyle;
	char const * lpszIcon;
	DWORD_PTR dwContextHelpId;
	void * lpfnMsgBoxCallback;
	DWORD dwLanguageId;
};

// Implemented in sdlstub.cpp; neither supports an owner window, a help callback, or a
// language selection.
int MessageBoxA(HWND owner, char const * text, char const * caption, UINT type);
int MessageBoxIndirect(MSGBOXPARAMS const * params);
#define MessageBox MessageBoxA

// Reached only through UnusedWindow, which nothing on this build ever sets to a real handle.
inline BOOL CloseWindow(HWND) { return(TRUE); }
inline LRESULT DefWindowProcW(HWND, UINT, WPARAM, LPARAM) { return(0); }

// The renderer presents every frame regardless of any OS-level dirty rect, so there is
// nothing for a repaint request to trigger.
inline BOOL InvalidateRect(HWND, void const *, BOOL) { return(TRUE); }

// SDL reports display bounds per display rather than a single desktop metric; this answers
// for the primary display, which is what every caller here already assumes.
int GetSystemMetrics(int index);

// Only the fields EnumDisplayModes reads; the real DEVMODE carries many more.
struct DEVMODE
{
	DWORD dmSize;
	DWORD dmPelsWidth;
	DWORD dmPelsHeight;
};

// lpszDeviceName is unused; every query answers for the primary display, which is the
// only one this build ever asks about.
BOOL EnumDisplaySettings(char const * device_name, int mode_index, DEVMODE * devmode);

// There is no window device context on Linux; the caller (WS_Get_Font) already treats a
// null context as "skip drawing with GDI."
inline HDC GetDC(HWND) {return(NULL);}
inline int ReleaseDC(HWND, HDC) {return(1);}

HWND SetFocus(HWND window);
BOOL PostMessage(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

// Nothing on this build runs a GetMessage loop that a WM_QUIT would stop.
inline void PostQuitMessage(int) {}
HWND SetCapture(HWND window);
BOOL ReleaseCapture(void);
HWND GetCapture(void);

// A repeating timer, matching real SetTimer; the message pump fires WM_TIMER for one whose
// deadline has passed and reschedules it, so no callback function pointer is supported.
UINT_PTR SetTimer(HWND window, UINT_PTR id, UINT elapse, void * callback);
BOOL KillTimer(HWND window, UINT_PTR id);

// Client/screen coordinates are both the desktop's pixel space; the window's own top-left
// is the only offset between them.
BOOL ClientToScreen(HWND window, POINT * point);
BOOL ScreenToClient(HWND window, POINT * point);
BOOL GetWindowRect(HWND window, RECT * rect);
BOOL GetClientRect(HWND window, RECT * rect);
BOOL IsIconic(HWND window);

// Wayland's security model refuses to warp the pointer outside an input-locked surface, so
// SetCursorPos silently does nothing there; X11 and other backends move it as asked.
BOOL SetCursorPos(int x, int y);
BOOL GetCursorPos(POINT * point);

// Confines the OS cursor to a screen-space rect on the main window, or releases it for NULL.
BOOL ClipCursor(RECT const * rect);

// Matches ShowCursor's real Win32 contract: TRUE/FALSE move a signed display counter by one
// and return the result, and the cursor is only actually drawn while the counter is >= 0.
int ShowCursor(BOOL show);

// A VK already names a physical key in this build, so MapVirtualKey is an identity map and
// ToUnicode has no dead-key composition.
SHORT GetKeyState(int vk);
SHORT GetAsyncKeyState(int vk);
UINT MapVirtualKey(UINT code, UINT maptype);
int ToUnicode(UINT vk, UINT scancode, PBYTE keystate, LPWSTR buffer, int buffer_count, UINT flags);
int GetKeyNameText(LONG lparam, char * buffer, int buffer_count);

#define IS_HIGH_SURROGATE(wch) ((wch) >= 0xD800 && (wch) <= 0xDBFF)
#define IS_LOW_SURROGATE(wch) ((wch) >= 0xDC00 && (wch) <= 0xDFFF)
#define IS_SURROGATE_PAIR(hs, ls) (IS_HIGH_SURROGATE(hs) && IS_LOW_SURROGATE(ls))

// Backs timeGetTime/QueryPerformanceCounter with std::chrono::steady_clock, so the tree's
// scattered timing call sites (mainloop.cpp, gametime.cpp, video.cpp, and similar) need no
// change; rmlsystem.cpp already uses steady_clock directly for the same purpose.
inline DWORD timeGetTime(void)
{
	auto const now = std::chrono::steady_clock::now().time_since_epoch();
	return((DWORD)std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

inline DWORD GetTickCount64(void) { return(timeGetTime()); }

union LARGE_INTEGER
{
	struct
	{
		DWORD LowPart;
		LONG HighPart;
	};
	std::int64_t QuadPart;
};

inline BOOL QueryPerformanceCounter(LARGE_INTEGER * counter)
{
	auto const now = std::chrono::steady_clock::now().time_since_epoch();
	counter->QuadPart = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
	return(TRUE);
}

inline BOOL QueryPerformanceFrequency(LARGE_INTEGER * frequency)
{
	frequency->QuadPart = 1000000000LL;
	return(TRUE);
}

inline void Sleep(DWORD milliseconds)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

// Only Windows needs to ask for finer Sleep granularity; a POSIX sleep already has it.
inline void timeBeginPeriod(UINT) {}
inline void timeEndPeriod(UINT) {}

// FILETIME's 100ns-tick, two-DWORD layout matches savefile.cpp's on-disk field.
struct FILETIME
{
	DWORD dwLowDateTime;
	DWORD dwHighDateTime;
};

struct SYSTEMTIME
{
	WORD wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds;
};

inline constexpr std::uint64_t OPENTS_FILETIME_UNIX_EPOCH = 116444736000000000ULL;

inline std::uint64_t OpenTS_FileTime_To_Ticks(FILETIME const & ft)
{
	return((std::uint64_t(ft.dwHighDateTime) << 32) | ft.dwLowDateTime);
}

inline FILETIME OpenTS_Ticks_To_FileTime(std::uint64_t ticks)
{
	FILETIME ft;
	ft.dwLowDateTime = (DWORD)(ticks & 0xFFFFFFFFu);
	ft.dwHighDateTime = (DWORD)(ticks >> 32);
	return(ft);
}

inline void GetSystemTimeAsFileTime(FILETIME * ft)
{
	auto const now = std::chrono::system_clock::now().time_since_epoch();
	std::uint64_t const ticks100ns = (std::uint64_t)
		std::chrono::duration_cast<std::chrono::duration<std::int64_t, std::ratio<1, 10000000>>>(now).count();
	*ft = OpenTS_Ticks_To_FileTime(ticks100ns + OPENTS_FILETIME_UNIX_EPOCH);
}

inline LONG CompareFileTime(FILETIME const * a, FILETIME const * b)
{
	std::uint64_t const left = OpenTS_FileTime_To_Ticks(*a);
	std::uint64_t const right = OpenTS_FileTime_To_Ticks(*b);
	return(left < right ? -1 : (left > right ? 1 : 0));
}

inline BOOL FileTimeToSystemTime(FILETIME const * ft, SYSTEMTIME * st)
{
	std::uint64_t const ticks = OpenTS_FileTime_To_Ticks(*ft);
	std::time_t const seconds = (std::time_t)((ticks - OPENTS_FILETIME_UNIX_EPOCH) / 10000000ULL);
	unsigned const ms = (unsigned)(((ticks - OPENTS_FILETIME_UNIX_EPOCH) / 10000ULL) % 1000);

	std::tm utc{};
	if (gmtime_r(&seconds, &utc) == nullptr) {
		return(FALSE);
	}

	st->wYear = (WORD)(utc.tm_year + 1900);
	st->wMonth = (WORD)(utc.tm_mon + 1);
	st->wDayOfWeek = (WORD)utc.tm_wday;
	st->wDay = (WORD)utc.tm_mday;
	st->wHour = (WORD)utc.tm_hour;
	st->wMinute = (WORD)utc.tm_min;
	st->wSecond = (WORD)utc.tm_sec;
	st->wMilliseconds = (WORD)ms;
	return(TRUE);
}

inline BOOL SystemTimeToFileTime(SYSTEMTIME const * st, FILETIME * ft)
{
	std::tm utc{};
	utc.tm_year = st->wYear - 1900;
	utc.tm_mon = st->wMonth - 1;
	utc.tm_mday = st->wDay;
	utc.tm_hour = st->wHour;
	utc.tm_min = st->wMinute;
	utc.tm_sec = st->wSecond;
	utc.tm_isdst = 0;

	std::time_t const seconds = timegm(&utc);
	if (seconds == (std::time_t)-1) {
		return(FALSE);
	}

	std::uint64_t const ticks = (std::uint64_t)seconds * 10000000ULL
		+ (std::uint64_t)st->wMilliseconds * 10000ULL + OPENTS_FILETIME_UNIX_EPOCH;
	*ft = OpenTS_Ticks_To_FileTime(ticks);
	return(TRUE);
}

inline void GetSystemTime(SYSTEMTIME * st)
{
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	FileTimeToSystemTime(&ft, st);
}

// Logged for the out-of-sync report only; the x87 control-word format it names does not
// carry over to this build's ABI, so the query side always reads zero.
inline unsigned int _controlfp(unsigned int, unsigned int)
{
	return(0);
}

// Windows reports this in the process's local time zone; std::chrono's local_t makes that
// query portable without hand-rolling zone-offset math.
inline BOOL FileTimeToLocalFileTime(FILETIME const * ft, FILETIME * local)
{
	std::uint64_t const ticks = OpenTS_FileTime_To_Ticks(*ft);
	std::time_t const utc_seconds = (std::time_t)((ticks - OPENTS_FILETIME_UNIX_EPOCH) / 10000000ULL);

	std::tm local_tm{};
	if (localtime_r(&utc_seconds, &local_tm) == nullptr) {
		return(FALSE);
	}

	std::time_t const local_seconds = timegm(&local_tm);
	std::uint64_t const new_ticks = (std::uint64_t)local_seconds * 10000000ULL
		+ (ticks % 10000000ULL) + OPENTS_FILETIME_UNIX_EPOCH;
	*local = OpenTS_Ticks_To_FileTime(new_ticks);
	return(TRUE);
}

inline void GetLocalTime(SYSTEMTIME * st)
{
	FILETIME ft;
	FILETIME local;
	GetSystemTimeAsFileTime(&ft);
	FileTimeToLocalFileTime(&ft, &local);
	FileTimeToSystemTime(&local, st);
}

using LCID = DWORD;
#define LANG_USER_DEFAULT 0
#define TIME_NOSECONDS 0x00000002
#define TIME_NOMINUTESORSECONDS 0x00000004

inline int GetDateFormat(LCID, DWORD, SYSTEMTIME const * date, char const *, char * out, int out_size)
{
	std::tm tm{};
	tm.tm_year = date->wYear - 1900;
	tm.tm_mon = date->wMonth - 1;
	tm.tm_mday = date->wDay;
	tm.tm_wday = date->wDayOfWeek;

	std::size_t const written = std::strftime(out, (std::size_t)out_size, "%x", &tm);
	return((int)written);
}

// TIME_NOSECONDS and TIME_NOMINUTESORSECONDS are the only flags any caller passes, so
// minutes are always shown and only the seconds field is conditional.
inline int GetTimeFormat(LCID, DWORD flags, SYSTEMTIME const * time, char const *, char * out, int out_size)
{
	std::tm tm{};
	tm.tm_hour = time->wHour;
	tm.tm_min = time->wMinute;
	tm.tm_sec = time->wSecond;

	char const * const format = (flags & (TIME_NOSECONDS | TIME_NOMINUTESORSECONDS)) != 0 ? "%H:%M" : "%X";
	std::size_t const written = std::strftime(out, (std::size_t)out_size, format, &tm);
	return((int)written);
}

#define FILE_ATTRIBUTE_NORMAL    0x00000080
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#define FILE_ATTRIBUTE_HIDDEN    0x00000002
#define FILE_ATTRIBUTE_SYSTEM    0x00000004
#define FILE_ATTRIBUTE_TEMPORARY 0x00000100
#define INVALID_FILE_ATTRIBUTES  ((DWORD)-1)
#define INVALID_HANDLE_VALUE     ((HANDLE)(std::intptr_t)-1)

inline DWORD GetFileAttributes(char const * path)
{
	struct stat info;
	if (stat(path, &info) != 0) {
		return(INVALID_FILE_ATTRIBUTES);
	}
	return(S_ISDIR(info.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL);
}
#define GetFileAttributesA GetFileAttributes

struct WIN32_FILE_ATTRIBUTE_DATA
{
	DWORD dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	DWORD nFileSizeHigh;
	DWORD nFileSizeLow;
};

enum GET_FILEEX_INFO_LEVELS
{
	GetFileExInfoStandard,
};

inline BOOL GetFileAttributesEx(char const * path, GET_FILEEX_INFO_LEVELS, void * data)
{
	struct stat info;
	if (stat(path, &info) != 0) {
		return(FALSE);
	}

	auto const to_ticks = [](time_t seconds) {
		return(OpenTS_Ticks_To_FileTime((std::uint64_t)seconds * 10000000ULL + OPENTS_FILETIME_UNIX_EPOCH));
	};

	WIN32_FILE_ATTRIBUTE_DATA * out = (WIN32_FILE_ATTRIBUTE_DATA *)data;
	out->dwFileAttributes = S_ISDIR(info.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
	out->ftCreationTime = to_ticks(info.st_ctime);
	out->ftLastAccessTime = to_ticks(info.st_atime);
	out->ftLastWriteTime = to_ticks(info.st_mtime);
	out->nFileSizeHigh = (DWORD)((std::uint64_t)info.st_size >> 32);
	out->nFileSizeLow = (DWORD)(info.st_size & 0xFFFFFFFFu);
	return(TRUE);
}

struct MEMORYSTATUS
{
	DWORD dwLength;
	DWORD dwMemoryLoad;
	DWORD dwTotalPhys;
	DWORD dwAvailPhys;
	DWORD dwTotalPageFile;
	DWORD dwAvailPageFile;
	DWORD dwTotalVirtual;
	DWORD dwAvailVirtual;
};

inline void GlobalMemoryStatus(MEMORYSTATUS * status)
{
	struct sysinfo info;
	std::memset(status, 0, sizeof(*status));
	status->dwLength = sizeof(*status);
	if (sysinfo(&info) != 0) {
		return;
	}

	std::uint64_t const total_phys = (std::uint64_t)info.totalram * info.mem_unit;
	std::uint64_t const avail_phys = (std::uint64_t)info.freeram * info.mem_unit;
	std::uint64_t const total_swap = (std::uint64_t)info.totalswap * info.mem_unit;
	std::uint64_t const avail_swap = (std::uint64_t)info.freeswap * info.mem_unit;

	status->dwMemoryLoad = total_phys > 0 ? (DWORD)(100 - (avail_phys * 100 / total_phys)) : 0;
	status->dwTotalPhys = (DWORD)std::min<std::uint64_t>(total_phys, 0xFFFFFFFFu);
	status->dwAvailPhys = (DWORD)std::min<std::uint64_t>(avail_phys, 0xFFFFFFFFu);
	status->dwTotalPageFile = (DWORD)std::min<std::uint64_t>(total_phys + total_swap, 0xFFFFFFFFu);
	status->dwAvailPageFile = (DWORD)std::min<std::uint64_t>(avail_phys + avail_swap, 0xFFFFFFFFu);
	status->dwTotalVirtual = status->dwTotalPageFile;
	status->dwAvailVirtual = status->dwAvailPageFile;
}

// There is one module on this build: the executable itself, resolved through /proc/self/exe.
// A caller-supplied handle is ignored, matching GetModuleHandle(nullptr)'s own meaning.
inline HMODULE GetModuleHandle(char const *)
{
	return((HMODULE)1);
}
#define GetModuleHandleA GetModuleHandle

inline DWORD GetModuleFileName(HMODULE, char * buffer, DWORD buffer_size)
{
	ssize_t const written = readlink("/proc/self/exe", buffer, buffer_size > 0 ? (std::size_t)buffer_size - 1 : 0);
	if (written < 0) {
		return(0);
	}

	buffer[written] = '\0';
	return((DWORD)written);
}
#define GetModuleFileNameA GetModuleFileName

inline BOOL CreateDirectory(char const * path, void *)
{
	return((mkdir(path, 0755) == 0 || errno == EEXIST) ? TRUE : FALSE);
}
#define CreateDirectoryA CreateDirectory

struct WIN32_FIND_DATA
{
	DWORD dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	DWORD nFileSizeHigh;
	DWORD nFileSizeLow;
	char cFileName[260];
	char cAlternateFileName[14];
};
using WIN32_FIND_DATAA = WIN32_FIND_DATA;

// Matches Win32 wildcard semantics closely enough for the game's own search patterns
// ("*.mix", "*.ini", and similar): '*' matches any run of characters, '?' matches one, and
// the comparison is case-insensitive.
inline bool OpenTS_Match_Wildcard(char const * pattern, char const * name)
{
	if (*pattern == '\0') {
		return(*name == '\0');
	}
	if (*pattern == '*') {
		while (*pattern == '*') pattern++;
		if (*pattern == '\0') {
			return(true);
		}
		for (char const * scan = name; *scan != '\0'; scan++) {
			if (OpenTS_Match_Wildcard(pattern, scan)) {
				return(true);
			}
		}
		return(OpenTS_Match_Wildcard(pattern, name + std::strlen(name)));
	}
	if (*name == '\0') {
		return(false);
	}
	if (*pattern == '?' || std::tolower((unsigned char)*pattern) == std::tolower((unsigned char)*name)) {
		return(OpenTS_Match_Wildcard(pattern + 1, name + 1));
	}
	return(false);
}

struct OpenTSFindHandle
{
	DIR * Directory;
	std::string DirectoryPath;
	std::string Pattern;
};

inline void OpenTS_Fill_Find_Data(std::string const & directory, char const * name, WIN32_FIND_DATA & data)
{
	std::memset(&data, 0, sizeof(data));
	std::strncpy(data.cFileName, name, sizeof(data.cFileName) - 1);

	struct stat info;
	if (stat((directory + "/" + name).c_str(), &info) == 0) {
		data.dwFileAttributes = S_ISDIR(info.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
		data.nFileSizeLow = (DWORD)(info.st_size & 0xFFFFFFFFu);
		data.nFileSizeHigh = (DWORD)((std::uint64_t)info.st_size >> 32);

		auto const to_ticks = [](time_t seconds) {
			return(OpenTS_Ticks_To_FileTime((std::uint64_t)seconds * 10000000ULL + OPENTS_FILETIME_UNIX_EPOCH));
		};
		data.ftLastWriteTime = to_ticks(info.st_mtime);
		data.ftLastAccessTime = to_ticks(info.st_atime);
		data.ftCreationTime = to_ticks(info.st_ctime);
	}
}

// Splits a Win32-style "<directory>\<pattern>" search spec (also accepting '/') into the
// directory to scan and the wildcard pattern to match within it.
inline void OpenTS_Split_Search_Path(std::string const & search, std::string & directory, std::string & pattern)
{
	std::string::size_type const slash = search.find_last_of("\\/");
	if (slash == std::string::npos) {
		directory = ".";
		pattern = search;
	} else {
		directory = search.substr(0, slash);
		pattern = search.substr(slash + 1);
		if (directory.empty()) {
			directory = "/";
		}
	}
}

inline HANDLE FindFirstFile(char const * search, WIN32_FIND_DATA * data)
{
	std::string directory, pattern;
	OpenTS_Split_Search_Path(search, directory, pattern);

	DIR * dir = opendir(directory.c_str());
	if (dir == nullptr) {
		return(INVALID_HANDLE_VALUE);
	}

	OpenTSFindHandle * handle = new OpenTSFindHandle{ dir, directory, pattern };

	struct dirent * entry;
	while ((entry = readdir(dir)) != nullptr) {
		if (OpenTS_Match_Wildcard(pattern.c_str(), entry->d_name)) {
			OpenTS_Fill_Find_Data(directory, entry->d_name, *data);
			return((HANDLE)handle);
		}
	}

	closedir(dir);
	delete handle;
	return(INVALID_HANDLE_VALUE);
}

inline BOOL FindNextFile(HANDLE handle, WIN32_FIND_DATA * data)
{
	OpenTSFindHandle * find = static_cast<OpenTSFindHandle *>(handle);

	struct dirent * entry;
	while ((entry = readdir(find->Directory)) != nullptr) {
		if (OpenTS_Match_Wildcard(find->Pattern.c_str(), entry->d_name)) {
			OpenTS_Fill_Find_Data(find->DirectoryPath, entry->d_name, *data);
			return(TRUE);
		}
	}
	return(FALSE);
}

inline BOOL FindClose(HANDLE handle)
{
	OpenTSFindHandle * find = static_cast<OpenTSFindHandle *>(handle);
	closedir(find->Directory);
	delete find;
	return(TRUE);
}

inline BOOL DeleteFile(char const * path)
{
	return(std::remove(path) == 0 ? TRUE : FALSE);
}

inline BOOL CopyFile(char const * existing, char const * dest, BOOL fail_if_exists)
{
	if (fail_if_exists) {
		struct stat info;
		if (stat(dest, &info) == 0) {
			return(FALSE);
		}
	}

	std::FILE * in = std::fopen(existing, "rb");
	if (in == nullptr) {
		return(FALSE);
	}

	std::FILE * out = std::fopen(dest, "wb");
	if (out == nullptr) {
		std::fclose(in);
		return(FALSE);
	}

	char buffer[65536];
	std::size_t read_count;
	bool ok = true;
	while ((read_count = std::fread(buffer, 1, sizeof(buffer), in)) > 0) {
		if (std::fwrite(buffer, 1, read_count, out) != read_count) {
			ok = false;
			break;
		}
	}
	ok = ok && std::feof(in) != 0;

	std::fclose(in);
	std::fclose(out);
	return(ok ? TRUE : FALSE);
}

#define GENERIC_READ  0x80000000u
#define GENERIC_WRITE 0x40000000u
#define FILE_SHARE_READ  0x00000001u
#define FILE_SHARE_WRITE 0x00000002u
#define OPEN_EXISTING 3u
#define CREATE_ALWAYS 2u
#define MOVEFILE_REPLACE_EXISTING 0x00000001u
#define INVALID_FILE_SIZE ((DWORD)0xFFFFFFFFu)

// Every caller here opens either an existing file for reading or a fresh one for writing,
// so only those two GENERIC_*/disposition combinations are recognized.
inline HANDLE CreateFileA(char const * path, DWORD access, DWORD, void const *, DWORD disposition, DWORD, HANDLE)
{
	char const * mode;
	if ((access & GENERIC_WRITE) != 0 && disposition == CREATE_ALWAYS) {
		mode = "wb";
	} else if ((access & GENERIC_READ) != 0 && disposition == OPEN_EXISTING) {
		mode = "rb";
	} else {
		return(INVALID_HANDLE_VALUE);
	}

	std::FILE * file = std::fopen(path, mode);
	return(file != nullptr ? (HANDLE)file : INVALID_HANDLE_VALUE);
}
#define CreateFile CreateFileA

inline BOOL ReadFile(HANDLE file, void * buffer, DWORD length, DWORD * got, void const *)
{
	std::size_t const read_count = std::fread(buffer, 1, length, (std::FILE *)file);
	if (got != nullptr) {
		*got = (DWORD)read_count;
	}
	return(std::ferror((std::FILE *)file) == 0 ? TRUE : FALSE);
}

inline BOOL WriteFile(HANDLE file, void const * buffer, DWORD length, DWORD * written, void const *)
{
	std::size_t const write_count = std::fwrite(buffer, 1, length, (std::FILE *)file);
	if (written != nullptr) {
		*written = (DWORD)write_count;
	}
	return((DWORD)write_count == length ? TRUE : FALSE);
}

inline BOOL FlushFileBuffers(HANDLE file)
{
	return(std::fflush((std::FILE *)file) == 0 ? TRUE : FALSE);
}

// Only ever called on a handle CreateFileA returned, so closing it is always an fclose.
inline BOOL CloseHandle(HANDLE file)
{
	if (file == nullptr || file == INVALID_HANDLE_VALUE) {
		return(FALSE);
	}
	return(std::fclose((std::FILE *)file) == 0 ? TRUE : FALSE);
}

inline DWORD GetFileSize(HANDLE file, DWORD * high)
{
	std::FILE * const stream = (std::FILE *)file;
	long const current = std::ftell(stream);
	if (current < 0 || std::fseek(stream, 0, SEEK_END) != 0) {
		return(INVALID_FILE_SIZE);
	}

	long const size = std::ftell(stream);
	std::fseek(stream, current, SEEK_SET);
	if (size < 0) {
		return(INVALID_FILE_SIZE);
	}

	if (high != nullptr) {
		*high = 0;
	}
	return((DWORD)size);
}

inline BOOL MoveFileExA(char const * existing, char const * dest, DWORD)
{
	return(std::rename(existing, dest) == 0 ? TRUE : FALSE);
}
#define MoveFileEx MoveFileExA

inline BOOL DeleteFileA(char const * path)
{
	return(DeleteFile(path));
}

inline BOOL SetCurrentDirectory(char const * path)
{
	return(chdir(path) == 0 ? TRUE : FALSE);
}

inline DWORD GetCurrentDirectory(DWORD size, char * buffer)
{
	if (getcwd(buffer, size) == nullptr) {
		return(0);
	}
	return((DWORD)std::strlen(buffer));
}

inline BOOL RemoveDirectory(char const * path)
{
	return(rmdir(path) == 0 ? TRUE : FALSE);
}

inline DWORD GetTempPath(DWORD size, char * buffer)
{
	char const * tmpdir = std::getenv("TMPDIR");
	if (tmpdir == nullptr || tmpdir[0] == '\0') {
		tmpdir = "/tmp";
	}

	int written = std::snprintf(buffer, size, "%s/", tmpdir);
	if (written < 0 || (DWORD)written >= size) {
		return(0);
	}
	return((DWORD)written);
}

inline DWORD GetCurrentProcessId(void)
{
	return((DWORD)getpid());
}

// Language.dll never loads on this build, so nothing real is ever handed here to free.
inline BOOL FreeLibrary(HMODULE) { return(TRUE); }

// The classic BMP file layout; real Windows headers wrap these the same way so the
// structures stay exactly 14 and 40 bytes with no compiler-dependent padding.
#pragma pack(push, 1)
struct BITMAPFILEHEADER
{
	WORD bfType;
	DWORD bfSize;
	WORD bfReserved1;
	WORD bfReserved2;
	DWORD bfOffBits;
};

struct BITMAPINFOHEADER
{
	DWORD biSize;
	LONG biWidth;
	LONG biHeight;
	WORD biPlanes;
	WORD biBitCount;
	DWORD biCompression;
	DWORD biSizeImage;
	LONG biXPelsPerMeter;
	LONG biYPelsPerMeter;
	DWORD biClrUsed;
	DWORD biClrImportant;
};

struct RGBQUAD
{
	BYTE rgbBlue;
	BYTE rgbGreen;
	BYTE rgbRed;
	BYTE rgbReserved;
};

struct BITMAPINFO
{
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD bmiColors[1];
};
#pragma pack(pop)

// Emulates MSVC's __declspec(property(...)), which GCC and Clang do not support, by
// recovering the owner's address from the property's own offset within it.
#define OPENTS_PROPERTY_PUSH \
	_Pragma("GCC diagnostic push") \
	_Pragma("GCC diagnostic ignored \"-Winvalid-offsetof\"")
#define OPENTS_PROPERTY_POP _Pragma("GCC diagnostic pop")

// operator-> covers pointer chaining and operator==/!= let a property sit inside another
// type's own defaulted comparison. A value-typed property's member access or arithmetic
// still needs a direct getter call at the site.
#define OPENTS_GET_PROPERTY(Owner, Type, Name, Getter) \
	struct Name##_PropertyType { \
		operator Type() const { \
			Owner const * owner = reinterpret_cast<Owner const *>( \
				reinterpret_cast<char const *>(this) - offsetof(Owner, Name)); \
			return owner->Getter(); \
		} \
		Type operator->() const { return (Type)(*this); } \
		bool operator==(Name##_PropertyType const & that) const { return (Type)(*this) == (Type)that; } \
		bool operator!=(Name##_PropertyType const & that) const { return (Type)(*this) != (Type)that; } \
	} Name

#define OPENTS_GET_SET_PROPERTY(Owner, Type, Name, Getter, Setter) \
	struct Name##_PropertyType { \
		operator Type() const { \
			Owner const * owner = reinterpret_cast<Owner const *>( \
				reinterpret_cast<char const *>(this) - offsetof(Owner, Name)); \
			return owner->Getter(); \
		} \
		Name##_PropertyType & operator=(Type value) { \
			Owner * owner = reinterpret_cast<Owner *>( \
				reinterpret_cast<char *>(this) - offsetof(Owner, Name)); \
			owner->Setter(value); \
			return *this; \
		} \
		Type operator->() const { return (Type)(*this); } \
		bool operator==(Name##_PropertyType const & that) const { return (Type)(*this) == (Type)that; } \
		bool operator!=(Name##_PropertyType const & that) const { return (Type)(*this) != (Type)that; } \
	} Name

#define OPENTS_GET_ARITH_PROPERTY(Owner, Type, Name, Getter) \
	struct Name##_PropertyType { \
		operator Type() const { \
			Owner const * owner = reinterpret_cast<Owner const *>( \
				reinterpret_cast<char const *>(this) - offsetof(Owner, Name)); \
			return owner->Getter(); \
		} \
		Type operator->() const { return (Type)(*this); } \
		bool operator==(Name##_PropertyType const & that) const { return (Type)(*this) == (Type)that; } \
		bool operator!=(Name##_PropertyType const & that) const { return (Type)(*this) != (Type)that; } \
		bool operator==(Type const & value) const { return (Type)(*this) == value; } \
		bool operator!=(Type const & value) const { return (Type)(*this) != value; } \
		Type operator+(Type const & value) const { return (Type)(*this) + value; } \
		Type operator-(Type const & value) const { return (Type)(*this) - value; } \
	} Name

// Adds == and != against Type, and + and - with Type on the right, for a coordinate-like
// property (Coord, Cell).
#define OPENTS_GET_SET_ARITH_PROPERTY(Owner, Type, Name, Getter, Setter) \
	struct Name##_PropertyType { \
		operator Type() const { \
			Owner const * owner = reinterpret_cast<Owner const *>( \
				reinterpret_cast<char const *>(this) - offsetof(Owner, Name)); \
			return owner->Getter(); \
		} \
		Name##_PropertyType & operator=(Type value) { \
			Owner * owner = reinterpret_cast<Owner *>( \
				reinterpret_cast<char *>(this) - offsetof(Owner, Name)); \
			owner->Setter(value); \
			return *this; \
		} \
		Type operator->() const { return (Type)(*this); } \
		bool operator==(Name##_PropertyType const & that) const { return (Type)(*this) == (Type)that; } \
		bool operator!=(Name##_PropertyType const & that) const { return (Type)(*this) != (Type)that; } \
		bool operator==(Type const & value) const { return (Type)(*this) == value; } \
		bool operator!=(Type const & value) const { return (Type)(*this) != value; } \
		Type operator+(Type const & value) const { return (Type)(*this) + value; } \
		Type operator-(Type const & value) const { return (Type)(*this) - value; } \
	} Name
