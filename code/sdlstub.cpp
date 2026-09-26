/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

// Translates SDL events into the same (message, wParam, lParam) triples the inherited
// Map, UIShell, and Keyboard handlers already expect, so those handlers stay unmodified.

#include "always.h"

#include "winstub.h"
#include "msgloop.h"

#include "_keyboar.h"
#include "_map.h"
#include "_tooltip.h"
#include "_ui.h"
#include "audio/audioengine.h"
#include "ccfile.h"
#include "cctooltip.h"
#include "convert.h"
#include "dbgprint.h"
#include "draw.h"
#include "except.h"
#include "gamewindow.h"
#include "globals.h"
#include "goptions.h"
#include "keyboard.h"
#include "misc.h"
#include "movie.h"
#include "pcx.h"
#include "queue.h"
#include "session.h"
#include "ui/uishell.h"
#include "video.h"
#include "vidscale.h"
#include "win.h"
#include "winfix.h"
#include "wwmouse.h"

#include "nativewindow.hh"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <sys/statvfs.h>
#include <vector>

int			ShowCommand;
HWND		MainWindow;
HWND		UnusedWindow;

HINSTANCE	ProgramInstance;
bool _MouseCaptured;

extern bool InMovie;
extern void VQA_PauseAudio(void);
extern void VQA_ResumeAudio(void);


void Focus_Loss(void)
{
	DebugString("Focus_Loss()\n");
	Pause_Ingame_Movie(true);
	AudioEngine.Focus_Loss();
	if (MouseCursor) {
		_MouseCaptured = MouseCursor->Is_Captured();
		MouseCursor->Release_Mouse();
	}
}


void Focus_Restore(void)
{
	DebugString("Focus_Restore()\n");
	AudioEngine.Focus_Restore();
	if (MouseCursor && _MouseCaptured == true && !Debug_Map) {
		MouseCursor->Capture_Mouse();
	}
	Map.Flag_To_Redraw(GS_REDRAW_ALL);
	Pause_Ingame_Movie(false);
}


namespace {

bool Is_Mouse_Coordinate_Message(UINT message)
{
	switch (message) {
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MBUTTONDBLCLK:
		case WM_MOUSEWHEEL:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
		case WM_XBUTTONDBLCLK:
			return(true);

		default:
			return(false);
	}
}


LPARAM Frame_Mouse_LParam(UINT message, LPARAM lparam)
{
	if (MainWindow == NULL || !Is_Mouse_Coordinate_Message(message) || !Video_Scaling_Active()) {
		return(lparam);
	}

	POINT point;
	point.x = GET_X_LPARAM(lparam);
	point.y = GET_Y_LPARAM(lparam);

	Window_Point_To_Game(point);

	return(MAKELPARAM((short)point.x, (short)point.y));
}


// UIShell and Map get first refusal at a message, a handful of messages drive window and
// video state directly, and Keyboard gets whatever is left.
void Dispatch_Message(UINT message, WPARAM wParam, LPARAM lParam)
{
	LPARAM const client_lparam = lParam;
	lParam = Frame_Mouse_LParam(message, lParam);

	if (UIShell.Handle_Window_Message(MainWindow, message, wParam, client_lparam)) {
		return;
	}

	Map.Message_Handler(MainWindow, message, wParam, lParam);

	switch (message) {
		case WM_PAINT:
			Game_Window_On_Paint(GameInFocus == true || WindowedMode == true);
			return;

		case WM_SIZE: {
			int width = 0;
			int height = 0;
			if (Win_Window_Drawable_Size(MainWindow, width, height)) {
				Video_On_Resize(width, height);
				Video_Set_Refresh_Rate(Win_Window_Refresh_Rate(MainWindow));
				if (MouseCursor != NULL) {
					((WWMouseClass *)MouseCursor)->Calc_Confining_Rect();
				}
			}
			return;
		}

		case WM_DISPLAYCHANGE:
			Video_Set_Refresh_Rate(Win_Window_Refresh_Rate(MainWindow));
			return;

		case WM_MOVE:
			if (WindowedMode == true && MouseCursor != NULL) {
				((WWMouseClass *)MouseCursor)->Calc_Confining_Rect();
			}
			return;

		case WM_ACTIVATEAPP:
			if (GameInFocus != (wParam != 0)) {
				GameInFocus = (wParam != 0);
				if (!GameInFocus) {
					Focus_Loss();
				} else {
					Focus_Restore();
				}
			}
			return;

		case WM_RBUTTONUP:
			Game_Window_On_Right_Mouse_Up();
			return;

		case WM_MOUSEWHEEL:
			Game_Window_On_Mouse_Wheel(GET_WHEEL_DELTA_WPARAM(wParam));
			return;

		case WM_SYSCOMMAND:
			if (wParam == SC_CLOSE) {
				// A running game resigns rather than closing, and keeps its window: the exit
				// is played through the queue, and the game ends itself once it arrives.
				if (GameActive && PlayerPtr != NULL && !Session.Play) {
					Queue_Exit();
				}
			}
			return;

		// The shutdown sequence posts this once it has decided to quit; startup.cpp's own
		// message loop waits on ReadyToQuit reaching 2 before it returns.
		case WM_DESTROY:
			if (ToolTips != NULL) {
				delete ToolTips;
				ToolTips = NULL;
			}
			MainWindow = NULL;

			if (ReadyToQuit == 1) {
				ReadyToQuit = 2;
			}
			return;
	}

	Keyboard->Message_Handler(MainWindow, message, wParam, lParam);
}


SDL_Window * Handle_Window(HWND window)
{
	return(static_cast<SDL_Window *>(window));
}


bool Is_VK_Down(int vk)
{
	switch (vk) {
		case VK_LBUTTON: return((SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_LMASK) != 0);
		case VK_RBUTTON: return((SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_RMASK) != 0);
		case VK_MBUTTON: return((SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_MMASK) != 0);
		case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:
			return((SDL_GetModState() & SDL_KMOD_SHIFT) != 0);
		case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
			return((SDL_GetModState() & SDL_KMOD_CTRL) != 0);
		case VK_MENU: case VK_LMENU: case VK_RMENU:
			return((SDL_GetModState() & SDL_KMOD_ALT) != 0);
		case VK_CAPITAL:
			return((SDL_GetModState() & SDL_KMOD_CAPS) != 0);
		case VK_NUMLOCK:
			return((SDL_GetModState() & SDL_KMOD_NUM) != 0);
		default: {
			SDL_Scancode const scancode = VK_To_Scancode((unsigned short)vk);
			if (scancode == SDL_SCANCODE_UNKNOWN) {
				return(false);
			}
			int count = 0;
			bool const * state = SDL_GetKeyboardState(&count);
			return(scancode < count && state[scancode]);
		}
	}
}


struct TimerEntry
{
	HWND Window;
	UINT_PTR Id;
	UINT Elapse;
	std::chrono::steady_clock::time_point Deadline;
};

std::vector<TimerEntry> Timers;

// Called from the poll loop; a real timer's callback runs on whichever thread dispatches
// its message, matching how this one runs on the thread that pumps events.
void Pump_Timers(void)
{
	auto const now = std::chrono::steady_clock::now();
	for (TimerEntry & timer : Timers) {
		if (now >= timer.Deadline) {
			timer.Deadline = now + std::chrono::milliseconds(timer.Elapse);
			Dispatch_Message(WM_TIMER, timer.Id, 0);
		}
	}
}

} // namespace


NativeWindow Win_Native_Window(HWND window)
{
	SDL_Window * sdl_window = Handle_Window(window);
	if (sdl_window == nullptr) {
		return(NativeWindow{ NATIVE_WINDOW_DEFAULT, nullptr, nullptr });
	}

	SDL_PropertiesID const properties = SDL_GetWindowProperties(sdl_window);

	void * wayland_surface = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
	if (wayland_surface != nullptr) {
		void * wayland_display = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
		return(NativeWindow{ NATIVE_WINDOW_WAYLAND, wayland_display, wayland_surface });
	}

	void * x11_window = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, nullptr);
	void * x11_display = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
	return(NativeWindow{ NATIVE_WINDOW_DEFAULT, x11_display, x11_window });
}


bool Win_Window_Drawable_Size(HWND window, int & width, int & height)
{
	SDL_Window * sdl_window = Handle_Window(window);
	if (sdl_window == nullptr || !SDL_GetWindowSizeInPixels(sdl_window, &width, &height)) {
		return(false);
	}

	return(width > 0 && height > 0);
}


int Win_Window_Refresh_Rate(HWND window)
{
	SDL_Window * sdl_window = Handle_Window(window);
	if (sdl_window == nullptr) {
		return(0);
	}

	SDL_DisplayID const display = SDL_GetDisplayForWindow(sdl_window);
	SDL_DisplayMode const * mode = SDL_GetCurrentDisplayMode(display);
	if (mode == nullptr || mode->refresh_rate <= 0.0f) {
		return(0);
	}

	return((int)(mode->refresh_rate + 0.5f));
}


void Win_Resize_And_Center_Window(HWND window, int width, int height)
{
	SDL_Window * sdl_window = Handle_Window(window);
	if (sdl_window == nullptr) {
		return;
	}

	int current_x = 0;
	int current_y = 0;
	int current_width = 0;
	int current_height = 0;
	SDL_GetWindowPosition(sdl_window, &current_x, &current_y);
	SDL_GetWindowSize(sdl_window, &current_width, &current_height);

	int x = current_x + ((current_width - width) / 2);
	int y = current_y + ((current_height - height) / 2);

	/*
	 * Growing about the middle can push the window past the edges of the screen, and a
	 * title bar above the top of it cannot be grabbed to bring the window back.
	 */
	SDL_Rect work{};
	if (SDL_GetDisplayUsableBounds(SDL_GetDisplayForWindow(sdl_window), &work)) {
		if (x + width > work.x + work.w) x = work.x + work.w - width;
		if (y + height > work.y + work.h) y = work.y + work.h - height;
		if (x < work.x) x = work.x;
		if (y < work.y) y = work.y;
	}

	SDL_SetWindowSize(sdl_window, width, height);
	SDL_SetWindowPosition(sdl_window, x, y);
}


HWND SetFocus(HWND window)
{
	SDL_Window * sdl_window = Handle_Window(window);
	if (sdl_window != nullptr) {
		SDL_RaiseWindow(sdl_window);
	}

	return(NULL);
}


// There is no separate OS message queue to post into, so the message reaches its handlers
// immediately rather than on the next pump.
BOOL PostMessage(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	if (window != MainWindow) {
		return(FALSE);
	}

	Dispatch_Message(message, wparam, lparam);
	return(TRUE);
}


namespace {
HWND CapturedWindow = NULL;
} // namespace


HWND SetCapture(HWND window)
{
	HWND const previous = CapturedWindow;

	SDL_Window * sdl_window = Handle_Window(window);
	if (sdl_window != nullptr) {
		SDL_CaptureMouse(true);
		CapturedWindow = window;
	}

	return(previous);
}


BOOL ReleaseCapture(void)
{
	SDL_CaptureMouse(false);
	CapturedWindow = NULL;
	return(TRUE);
}


HWND GetCapture(void)
{
	return(CapturedWindow);
}


namespace {
SDL_Cursor * SystemCursors[SDL_SYSTEM_CURSOR_COUNT] = {};

SDL_Cursor * Cached_System_Cursor(SDL_SystemCursor shape)
{
	if (SystemCursors[shape] == nullptr) {
		SystemCursors[shape] = SDL_CreateSystemCursor(shape);
	}
	return(SystemCursors[shape]);
}
} // namespace


HCURSOR LoadCursor(HINSTANCE, LPCTSTR id)
{
	WORD const value = (WORD)(std::uintptr_t)id;

	SDL_SystemCursor shape = SDL_SYSTEM_CURSOR_DEFAULT;
	switch (value) {
		case 32512: shape = SDL_SYSTEM_CURSOR_DEFAULT; break;
		case 32513: shape = SDL_SYSTEM_CURSOR_TEXT; break;
		case 32514: shape = SDL_SYSTEM_CURSOR_WAIT; break;
		case 32515: shape = SDL_SYSTEM_CURSOR_CROSSHAIR; break;
		case 32642: shape = SDL_SYSTEM_CURSOR_NWSE_RESIZE; break;
		case 32643: shape = SDL_SYSTEM_CURSOR_NESW_RESIZE; break;
		case 32644: shape = SDL_SYSTEM_CURSOR_EW_RESIZE; break;
		case 32645: shape = SDL_SYSTEM_CURSOR_NS_RESIZE; break;
		case 32646: shape = SDL_SYSTEM_CURSOR_MOVE; break;
		case 32648: shape = SDL_SYSTEM_CURSOR_NOT_ALLOWED; break;
		case 32649: shape = SDL_SYSTEM_CURSOR_POINTER; break;
		default: break;
	}

	return((HCURSOR)Cached_System_Cursor(shape));
}


void SetCursor(HCURSOR cursor)
{
	SDL_SetCursor((SDL_Cursor *)cursor);
}


void DestroyCursor(HCURSOR cursor)
{
	SDL_DestroyCursor((SDL_Cursor *)cursor);
}


UINT_PTR SetTimer(HWND window, UINT_PTR id, UINT elapse, void *)
{
	for (TimerEntry & timer : Timers) {
		if (timer.Window == window && timer.Id == id) {
			timer.Elapse = elapse;
			timer.Deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(elapse);
			return(id);
		}
	}

	Timers.push_back(TimerEntry{window, id, elapse,
		std::chrono::steady_clock::now() + std::chrono::milliseconds(elapse)});
	return(id);
}


BOOL KillTimer(HWND window, UINT_PTR id)
{
	for (auto it = Timers.begin(); it != Timers.end(); ++it) {
		if (it->Window == window && it->Id == id) {
			Timers.erase(it);
			return(TRUE);
		}
	}
	return(FALSE);
}


BOOL ClientToScreen(HWND window, POINT * point)
{
	SDL_Window * sdl_window = Handle_Window(window);
	if (sdl_window == nullptr || point == nullptr) {
		return(FALSE);
	}

	int window_x = 0;
	int window_y = 0;
	SDL_GetWindowPosition(sdl_window, &window_x, &window_y);

	point->x += window_x;
	point->y += window_y;
	return(TRUE);
}


BOOL ScreenToClient(HWND window, POINT * point)
{
	SDL_Window * sdl_window = Handle_Window(window);
	if (sdl_window == nullptr || point == nullptr) {
		return(FALSE);
	}

	int window_x = 0;
	int window_y = 0;
	SDL_GetWindowPosition(sdl_window, &window_x, &window_y);

	point->x -= window_x;
	point->y -= window_y;
	return(TRUE);
}


BOOL GetWindowRect(HWND window, RECT * rect)
{
	SDL_Window * sdl_window = Handle_Window(window);
	if (sdl_window == nullptr || rect == nullptr) {
		return(FALSE);
	}

	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;
	SDL_GetWindowPosition(sdl_window, &x, &y);
	SDL_GetWindowSize(sdl_window, &width, &height);

	rect->left = x;
	rect->top = y;
	rect->right = x + width;
	rect->bottom = y + height;
	return(TRUE);
}


BOOL IsIconic(HWND window)
{
	SDL_Window * sdl_window = Handle_Window(window);
	if (sdl_window == nullptr) {
		return(FALSE);
	}

	return((SDL_GetWindowFlags(sdl_window) & SDL_WINDOW_MINIMIZED) != 0 ? TRUE : FALSE);
}


BOOL GetClientRect(HWND window, RECT * rect)
{
	SDL_Window * sdl_window = Handle_Window(window);
	if (sdl_window == nullptr || rect == nullptr) {
		return(FALSE);
	}

	int width = 0;
	int height = 0;
	SDL_GetWindowSize(sdl_window, &width, &height);

	rect->left = 0;
	rect->top = 0;
	rect->right = width;
	rect->bottom = height;
	return(TRUE);
}


BOOL ClipCursor(RECT const * rect)
{
	SDL_Window * sdl_window = Handle_Window(MainWindow);
	if (sdl_window == nullptr) {
		return(FALSE);
	}

	if (rect == nullptr) {
		return(SDL_SetWindowMouseRect(sdl_window, nullptr));
	}

	int window_x = 0;
	int window_y = 0;
	SDL_GetWindowPosition(sdl_window, &window_x, &window_y);

	SDL_Rect confine;
	confine.x = rect->left - window_x;
	confine.y = rect->top - window_y;
	confine.w = rect->right - rect->left;
	confine.h = rect->bottom - rect->top;
	return(SDL_SetWindowMouseRect(sdl_window, &confine));
}


int ShowCursor(BOOL show)
{
	static int display_count = 0;

	display_count += show ? 1 : -1;

	if (display_count >= 0) {
		SDL_ShowCursor();
	} else {
		SDL_HideCursor();
	}

	return(display_count);
}


BOOL EnumDisplaySettings(char const *, int mode_index, DEVMODE * devmode)
{
	int count = 0;
	SDL_DisplayMode ** modes = SDL_GetFullscreenDisplayModes(SDL_GetPrimaryDisplay(), &count);
	if (modes == nullptr) {
		return(FALSE);
	}

	BOOL result = FALSE;
	if (mode_index >= 0 && mode_index < count) {
		devmode->dmPelsWidth = (DWORD)modes[mode_index]->w;
		devmode->dmPelsHeight = (DWORD)modes[mode_index]->h;
		result = TRUE;
	}

	SDL_free(modes);
	return(result);
}


BOOL SetCursorPos(int x, int y)
{
	return(SDL_WarpMouseGlobal((float)x, (float)y) ? TRUE : FALSE);
}


BOOL GetCursorPos(POINT * point)
{
	if (point == nullptr) {
		return(FALSE);
	}

	float x = 0.0f;
	float y = 0.0f;
	SDL_GetGlobalMouseState(&x, &y);
	point->x = (LONG)x;
	point->y = (LONG)y;
	return(TRUE);
}


SHORT GetKeyState(int vk)
{
	return(Is_VK_Down(vk) ? (SHORT)0x8000 : 0);
}


SHORT GetAsyncKeyState(int vk)
{
	return(Is_VK_Down(vk) ? (SHORT)0x8000 : 0);
}


UINT MapVirtualKey(UINT code, UINT)
{
	return(code);
}


// No dead-key composition; a VK with no direct character (function keys, arrows, and
// similar) returns 0, matching ToUnicode's own "no translation" result.
int ToUnicode(UINT vk, UINT, PBYTE keystate, LPWSTR buffer, int buffer_count, UINT)
{
	if (buffer_count < 1) {
		return(0);
	}

	bool const shifted = (keystate[VK_SHIFT] & 0x80) != 0;
	wchar_t ch = 0;

	if (vk >= VK_A && vk <= VK_Z) {
		ch = (wchar_t)(shifted ? vk : vk + ('a' - 'A'));
	} else if (vk >= VK_0 && vk <= VK_9) {
		static wchar_t const shifted_digits[] = L")!@#$%^&*(";
		ch = shifted ? shifted_digits[vk - VK_0] : (wchar_t)vk;
	} else {
		switch (vk) {
			case VK_SPACE: ch = L' '; break;
			case VK_OEM_MINUS: ch = shifted ? L'_' : L'-'; break;
			case VK_OEM_PLUS: ch = shifted ? L'+' : L'='; break;
			case VK_OEM_COMMA: ch = shifted ? L'<' : L','; break;
			case VK_OEM_PERIOD: ch = shifted ? L'>' : L'.'; break;
			case VK_OEM_1: ch = shifted ? L':' : L';'; break;
			case VK_OEM_2: ch = shifted ? L'?' : L'/'; break;
			case VK_OEM_3: ch = shifted ? L'~' : L'`'; break;
			case VK_OEM_4: ch = shifted ? L'{' : L'['; break;
			case VK_OEM_5: ch = shifted ? L'|' : L'\\'; break;
			case VK_OEM_6: ch = shifted ? L'}' : L']'; break;
			case VK_OEM_7: ch = shifted ? L'"' : L'\''; break;
			default: break;
		}
	}

	if (ch == 0) {
		return(0);
	}

	buffer[0] = ch;
	return(1);
}


// Callers build lparam the way a real WM_KEYDOWN would carry it, with the VK in bits 16-23;
// SDL's own scancode name stands in for the localized key name Windows would look up.
int GetKeyNameText(LONG lparam, char * buffer, int buffer_count)
{
	if (buffer_count < 1) {
		return(0);
	}

	unsigned short const vk = (unsigned short)((lparam >> 16) & 0xFF);
	SDL_Scancode const scancode = VK_To_Scancode(vk);
	char const * const name = scancode != SDL_SCANCODE_UNKNOWN ? SDL_GetScancodeName(scancode) : "";

	if (name == nullptr || name[0] == '\0') {
		buffer[0] = '\0';
		return(0);
	}

	std::strncpy(buffer, name, (std::size_t)buffer_count - 1);
	buffer[buffer_count - 1] = '\0';
	return((int)std::strlen(buffer));
}


unsigned int Build_Number(void)
{
	return(OPENTS_VERSION_PACKED);
}


void Create_Main_Window(HINSTANCE instance, int command_show, int width, int height)
{
	(void)instance;

	SDL_InitSubSystem(SDL_INIT_VIDEO);
	SDL_DisableScreenSaver();

	int clientwidth = width;
	int clientheight = height;
	SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;

	if (WindowedMode) {
		if (Options.WindowWidth > 0) clientwidth = Options.WindowWidth;
		if (Options.WindowHeight > 0) clientheight = Options.WindowHeight;
	} else {
		flags |= SDL_WINDOW_FULLSCREEN;
	}

	SDL_Window * window = SDL_CreateWindow("Tiberian Sun", clientwidth, clientheight, flags);
	MainWindow = window;
	ShowCommand = command_show;

	if (window != nullptr) {
		SDL_ShowWindow(window);
	}
}


void Windows_Message_Handler(void)
{
	if (MainWindow == nullptr) return;

	Pump_Timers();

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_EVENT_QUIT:
			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
				Dispatch_Message(WM_SYSCOMMAND, SC_CLOSE, 0);
				break;

			case SDL_EVENT_WINDOW_EXPOSED:
				Dispatch_Message(WM_PAINT, 0, 0);
				break;

			case SDL_EVENT_WINDOW_RESIZED:
			case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
				Dispatch_Message(WM_SIZE, 0, 0);
				break;

			case SDL_EVENT_WINDOW_MOVED:
				Dispatch_Message(WM_MOVE, 0, 0);
				break;

			case SDL_EVENT_WINDOW_FOCUS_GAINED:
				Dispatch_Message(WM_ACTIVATEAPP, 1, 0);
				break;

			case SDL_EVENT_WINDOW_FOCUS_LOST:
				Dispatch_Message(WM_ACTIVATEAPP, 0, 0);
				break;

			case SDL_EVENT_MOUSE_MOTION:
				Dispatch_Message(WM_MOUSEMOVE, 0,
					MAKELPARAM((short)event.motion.x, (short)event.motion.y));
				break;

			case SDL_EVENT_MOUSE_BUTTON_DOWN:
			case SDL_EVENT_MOUSE_BUTTON_UP: {
				bool const down = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
				UINT message = Mouse_Button_Message(event.button.button, down);
				if (down && event.button.clicks >= 2) {
					message = message + (WM_LBUTTONDBLCLK - WM_LBUTTONDOWN);
				}
				Dispatch_Message(message, 0,
					MAKELPARAM((short)event.button.x, (short)event.button.y));
				break;
			}

			case SDL_EVENT_MOUSE_WHEEL: {
				int const delta = (int)(event.wheel.y * 120.0f);
				Dispatch_Message(WM_MOUSEWHEEL, MAKELONG(0, delta), 0);
				break;
			}

			case SDL_EVENT_KEY_DOWN:
			case SDL_EVENT_KEY_UP: {
				bool const down = (event.type == SDL_EVENT_KEY_DOWN);
				unsigned short const vk = Scancode_To_VK(event.key.scancode);
				UINT const message = down
					? (event.key.mod & (SDL_KMOD_ALT) ? WM_SYSKEYDOWN : WM_KEYDOWN)
					: (event.key.mod & (SDL_KMOD_ALT) ? WM_SYSKEYUP : WM_KEYUP);
				Dispatch_Message(message, vk, 0);
				break;
			}

			default:
				break;
		}
	}

	Video_Present_If_Dirty();
}


BOOL GetDiskFreeSpaceEx(char const * path, ULARGE_INTEGER * free_available,
	ULARGE_INTEGER * total, ULARGE_INTEGER * total_free)
{
	struct statvfs info;
	if (statvfs((path != nullptr && path[0] != '\0') ? path : ".", &info) != 0) {
		return(FALSE);
	}

	std::uint64_t const block_size = info.f_frsize;
	if (free_available != nullptr) free_available->QuadPart = (std::uint64_t)info.f_bavail * block_size;
	if (total != nullptr) total->QuadPart = (std::uint64_t)info.f_blocks * block_size;
	if (total_free != nullptr) total_free->QuadPart = (std::uint64_t)info.f_bfree * block_size;
	return(TRUE);
}


DWORD GetLastError(void)
{
	return((DWORD)errno);
}


namespace {

SDL_MessageBoxFlags Message_Box_Flags(DWORD style)
{
	if (style & MB_ICONSTOP) return(SDL_MESSAGEBOX_ERROR);
	if (style & MB_ICONEXCLAMATION) return(SDL_MESSAGEBOX_WARNING);
	return(SDL_MESSAGEBOX_INFORMATION);
}

} // namespace


int MessageBoxA(HWND owner, char const * text, char const * caption, UINT type)
{
	SDL_Window * window = Handle_Window(owner);

	if ((type & MB_YESNO) == 0) {
		SDL_ShowSimpleMessageBox(Message_Box_Flags(type), caption, text, window);
		return(IDOK);
	}

	SDL_MessageBoxButtonData const buttons[] = {
		{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, IDNO, "No" },
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, IDYES, "Yes" },
	};
	SDL_MessageBoxData const box = {
		Message_Box_Flags(type), window, caption, text,
		SDL_arraysize(buttons), buttons, nullptr,
	};

	int button = IDNO;
	SDL_ShowMessageBox(&box, &button);
	return(button);
}


int MessageBoxIndirect(MSGBOXPARAMS const * params)
{
	return(MessageBoxA(params->hwndOwner, params->lpszText, params->lpszCaption, params->dwStyle));
}


int GetSystemMetrics(int index)
{
	SDL_DisplayID const display = SDL_GetPrimaryDisplay();
	SDL_Rect bounds{};
	SDL_GetDisplayBounds(display, &bounds);

	switch (index) {
		case SM_CXSCREEN: return(bounds.w);
		case SM_CYSCREEN: return(bounds.h);
		case SM_CXFULLSCREEN: return(bounds.w);
		case SM_CYFULLSCREEN: return(bounds.h);
		case SM_CXDRAG: return(4);
		case SM_CYDRAG: return(4);
		case SM_SWAPBUTTON: return(0);
		default: return(0);
	}
}


/// <summary>
/// Loads a title screen picture and centers it on the surface.
/// This routine is used by the startup and scenario loading sequences to put some
/// artwork on the screen while the game gets itself ready. A paletted picture is
/// drawn through a converter built from the palette supplied.
/// </summary>
/// <param name="name">The name of the picture file to load.</param>
/// <param name="surface">The surface to draw the title screen upon.</param>
/// <param name="palette">The palette to load the picture's colors into.</param>
void Load_Title_Screen(char const * name, Surface * surface, PaletteClass * palette)
{
	Surface * load_buffer;
	CCFileClass file(name);
	load_buffer = Read_PCX_File(file, palette);

	if (load_buffer) {
		int x = (surface->Get_Width() - load_buffer->Get_Width()) / 2;
		int y = (surface->Get_Height() - load_buffer->Get_Height()) / 2;
		if (palette && load_buffer->Bytes_Per_Pixel() == 1) {
			ConvertClass * drawer = new ConvertClass(*palette, *palette, *surface);
			Blit_Block(*surface, *drawer, *load_buffer, load_buffer->Get_Rect(), Point2D(x, y), surface->Get_Rect());
			delete drawer;
		} else {
			surface->Blit_From(surface->Get_Rect(), Rect(x, y, load_buffer->Get_Width(), load_buffer->Get_Height()), *load_buffer, load_buffer->Get_Rect(), load_buffer->Get_Rect());
		}
		delete load_buffer;
	}
}
