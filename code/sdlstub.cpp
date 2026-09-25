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

#include "_map.h"
#include "audio/audioengine.h"
#include "except.h"
#include "gamewindow.h"
#include "globals.h"
#include "goptions.h"
#include "misc.h"
#include "movie.h"
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
#include <sys/statvfs.h>

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
	}

	Keyboard->Message_Handler(MainWindow, message, wParam, lParam);
}


// Maps an SDL button number to its up/down message pair.
UINT Mouse_Button_Message(Uint8 button, bool down)
{
	switch (button) {
		case SDL_BUTTON_LEFT:   return(down ? WM_LBUTTONDOWN : WM_LBUTTONUP);
		case SDL_BUTTON_RIGHT:  return(down ? WM_RBUTTONDOWN : WM_RBUTTONUP);
		case SDL_BUTTON_MIDDLE: return(down ? WM_MBUTTONDOWN : WM_MBUTTONUP);
		default:                return(down ? WM_XBUTTONDOWN : WM_XBUTTONUP);
	}
}


// SDL scancodes name a physical key; the inherited handlers expect the VK_* codes
// code/keyboard.h already defines, keyed by the same US layout the engine has always shipped.
unsigned short Scancode_To_VK(SDL_Scancode scancode)
{
	switch (scancode) {
		case SDL_SCANCODE_A: return(VK_A);
		case SDL_SCANCODE_B: return(VK_B);
		case SDL_SCANCODE_C: return(VK_C);
		case SDL_SCANCODE_D: return(VK_D);
		case SDL_SCANCODE_E: return(VK_E);
		case SDL_SCANCODE_F: return(VK_F);
		case SDL_SCANCODE_G: return(VK_G);
		case SDL_SCANCODE_H: return(VK_H);
		case SDL_SCANCODE_I: return(VK_I);
		case SDL_SCANCODE_J: return(VK_J);
		case SDL_SCANCODE_K: return(VK_K);
		case SDL_SCANCODE_L: return(VK_L);
		case SDL_SCANCODE_M: return(VK_M);
		case SDL_SCANCODE_N: return(VK_N);
		case SDL_SCANCODE_O: return(VK_O);
		case SDL_SCANCODE_P: return(VK_P);
		case SDL_SCANCODE_Q: return(VK_Q);
		case SDL_SCANCODE_R: return(VK_R);
		case SDL_SCANCODE_S: return(VK_S);
		case SDL_SCANCODE_T: return(VK_T);
		case SDL_SCANCODE_U: return(VK_U);
		case SDL_SCANCODE_V: return(VK_V);
		case SDL_SCANCODE_W: return(VK_W);
		case SDL_SCANCODE_X: return(VK_X);
		case SDL_SCANCODE_Y: return(VK_Y);
		case SDL_SCANCODE_Z: return(VK_Z);
		case SDL_SCANCODE_0: return(VK_0);
		case SDL_SCANCODE_1: return(VK_1);
		case SDL_SCANCODE_2: return(VK_2);
		case SDL_SCANCODE_3: return(VK_3);
		case SDL_SCANCODE_4: return(VK_4);
		case SDL_SCANCODE_5: return(VK_5);
		case SDL_SCANCODE_6: return(VK_6);
		case SDL_SCANCODE_7: return(VK_7);
		case SDL_SCANCODE_8: return(VK_8);
		case SDL_SCANCODE_9: return(VK_9);
		case SDL_SCANCODE_RETURN: return(VK_RETURN);
		case SDL_SCANCODE_ESCAPE: return(VK_ESCAPE);
		case SDL_SCANCODE_BACKSPACE: return(VK_BACK);
		case SDL_SCANCODE_TAB: return(VK_TAB);
		case SDL_SCANCODE_SPACE: return(VK_SPACE);
		case SDL_SCANCODE_MINUS: return(VK_OEM_MINUS);
		case SDL_SCANCODE_EQUALS: return(VK_OEM_PLUS);
		case SDL_SCANCODE_COMMA: return(VK_OEM_COMMA);
		case SDL_SCANCODE_PERIOD: return(VK_OEM_PERIOD);
		case SDL_SCANCODE_CAPSLOCK: return(VK_CAPITAL);
		case SDL_SCANCODE_F1: return(VK_F1);
		case SDL_SCANCODE_F2: return(VK_F2);
		case SDL_SCANCODE_F3: return(VK_F3);
		case SDL_SCANCODE_F4: return(VK_F4);
		case SDL_SCANCODE_F5: return(VK_F5);
		case SDL_SCANCODE_F6: return(VK_F6);
		case SDL_SCANCODE_F7: return(VK_F7);
		case SDL_SCANCODE_F8: return(VK_F8);
		case SDL_SCANCODE_F9: return(VK_F9);
		case SDL_SCANCODE_F10: return(VK_F10);
		case SDL_SCANCODE_F11: return(VK_F11);
		case SDL_SCANCODE_F12: return(VK_F12);
		case SDL_SCANCODE_PRINTSCREEN: return(VK_SNAPSHOT);
		case SDL_SCANCODE_SCROLLLOCK: return(VK_SCROLL);
		case SDL_SCANCODE_PAUSE: return(VK_PAUSE);
		case SDL_SCANCODE_INSERT: return(VK_INSERT);
		case SDL_SCANCODE_HOME: return(VK_HOME);
		case SDL_SCANCODE_PAGEUP: return(VK_PRIOR);
		case SDL_SCANCODE_DELETE: return(VK_DELETE);
		case SDL_SCANCODE_END: return(VK_END);
		case SDL_SCANCODE_PAGEDOWN: return(VK_NEXT);
		case SDL_SCANCODE_RIGHT: return(VK_RIGHT);
		case SDL_SCANCODE_LEFT: return(VK_LEFT);
		case SDL_SCANCODE_DOWN: return(VK_DOWN);
		case SDL_SCANCODE_UP: return(VK_UP);
		case SDL_SCANCODE_KP_DIVIDE: return(VK_DIVIDE);
		case SDL_SCANCODE_KP_MULTIPLY: return(VK_MULTIPLY);
		case SDL_SCANCODE_KP_MINUS: return(VK_SUBTRACT);
		case SDL_SCANCODE_KP_PLUS: return(VK_ADD);
		case SDL_SCANCODE_KP_ENTER: return(VK_RETURN);
		case SDL_SCANCODE_KP_1: return(VK_NUMPAD1);
		case SDL_SCANCODE_KP_2: return(VK_NUMPAD2);
		case SDL_SCANCODE_KP_3: return(VK_NUMPAD3);
		case SDL_SCANCODE_KP_4: return(VK_NUMPAD4);
		case SDL_SCANCODE_KP_5: return(VK_NUMPAD5);
		case SDL_SCANCODE_KP_6: return(VK_NUMPAD6);
		case SDL_SCANCODE_KP_7: return(VK_NUMPAD7);
		case SDL_SCANCODE_KP_8: return(VK_NUMPAD8);
		case SDL_SCANCODE_KP_9: return(VK_NUMPAD9);
		case SDL_SCANCODE_KP_0: return(VK_NUMPAD0);
		case SDL_SCANCODE_KP_PERIOD: return(VK_DECIMAL);
		case SDL_SCANCODE_APPLICATION: return(VK_APPS);
		case SDL_SCANCODE_LCTRL: return(VK_LCONTROL);
		case SDL_SCANCODE_LSHIFT: return(VK_LSHIFT);
		case SDL_SCANCODE_LALT: return(VK_LMENU);
		case SDL_SCANCODE_LGUI: return(VK_LWIN);
		case SDL_SCANCODE_RCTRL: return(VK_RCONTROL);
		case SDL_SCANCODE_RSHIFT: return(VK_RSHIFT);
		case SDL_SCANCODE_RALT: return(VK_RMENU);
		case SDL_SCANCODE_RGUI: return(VK_RWIN);
		case SDL_SCANCODE_NUMLOCKCLEAR: return(VK_NUMLOCK);
		case SDL_SCANCODE_GRAVE: return(VK_OEM_3);
		case SDL_SCANCODE_LEFTBRACKET: return(VK_OEM_4);
		case SDL_SCANCODE_BACKSLASH: return(VK_OEM_5);
		case SDL_SCANCODE_RIGHTBRACKET: return(VK_OEM_6);
		case SDL_SCANCODE_APOSTROPHE: return(VK_OEM_7);
		case SDL_SCANCODE_SEMICOLON: return(VK_OEM_1);
		case SDL_SCANCODE_SLASH: return(VK_OEM_2);
		default: return(VK_NONE);
	}
}


SDL_Window * Handle_Window(HWND window)
{
	return(static_cast<SDL_Window *>(window));
}


// Built once from Scancode_To_VK, so a query for a VK this build's fixed layout never
// produces stays SDL_SCANCODE_UNKNOWN.
SDL_Scancode VK_To_Scancode(unsigned short vk)
{
	static SDL_Scancode table[256];
	static bool built = false;

	if (!built) {
		for (unsigned short i = 0; i < 256; i++) {
			table[i] = SDL_SCANCODE_UNKNOWN;
		}
		for (int scancode = SDL_SCANCODE_UNKNOWN; scancode < SDL_SCANCODE_COUNT; scancode++) {
			unsigned short const mapped = Scancode_To_VK((SDL_Scancode)scancode);
			if (mapped != VK_NONE && table[mapped] == SDL_SCANCODE_UNKNOWN) {
				table[mapped] = (SDL_Scancode)scancode;
			}
		}
		built = true;
	}

	return(table[vk]);
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


HWND SetFocus(HWND window)
{
	SDL_Window * sdl_window = Handle_Window(window);
	if (sdl_window != nullptr) {
		SDL_RaiseWindow(sdl_window);
	}

	return(NULL);
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
