/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

// The Linux host: an SDL3 window, its pointer and cursor, key state, message boxes and
// display modes. SDL abstracts X11 and Wayland alike, so this file does not choose between
// them; __linux__ rather than !defined(_WIN32) keeps it out of a macOS build, which has no
// host of its own in this tree.

#if defined(__linux__)

#include "always.h"

#include "hostwindow.h"

#include "_keyboar.h"
#include "gamewindow.h"
#include "globals.h"
#include "goptions.h"
#include "keyboard.h"
#include "mainwindow.h"
#include "misc.h"
#include "msgloop.h"
#include "nativewindow.hh"
#include "queue.h"
#include "session.h"
#include "vidscale.h"
#include "video.h"
#include "win.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdint>


struct HostCursor
{
	SDL_Cursor * Handle;
};


namespace {

SDL_Window * _Window = nullptr;

// Independent of Host_Set_Cursor's image: a caller nests Host_Show_Pointer the way ShowCursor
// does, and Host_Hide_Cursor/Host_Set_Cursor answer a single one-shot "what does it look like".
int _ShowCount = 0;
bool _ExplicitlyHidden = false;
SDL_Cursor * _CurrentCursor = nullptr;

bool _Confined = false;
bool _Captured = false;

int _LastWidth = 0;
int _LastHeight = 0;


// The VK_ codes keyboard.h declares are the Windows virtual-key numbers, not a scheme of this
// tree's own; letters share a contiguous run with SDL_SCANCODE_A..SDL_SCANCODE_Z, so only the
// rest (digits included, since SDL puts the 0 key after 9 rather than before 1) need a table.
struct KeyMapEntry
{
	unsigned short Key;
	SDL_Scancode Scancode;
};

KeyMapEntry const _KeyMap[] = {
	{ VK_0,			SDL_SCANCODE_0 },
	{ VK_1,			SDL_SCANCODE_1 },
	{ VK_2,			SDL_SCANCODE_2 },
	{ VK_3,			SDL_SCANCODE_3 },
	{ VK_4,			SDL_SCANCODE_4 },
	{ VK_5,			SDL_SCANCODE_5 },
	{ VK_6,			SDL_SCANCODE_6 },
	{ VK_7,			SDL_SCANCODE_7 },
	{ VK_8,			SDL_SCANCODE_8 },
	{ VK_9,			SDL_SCANCODE_9 },
	{ VK_BACK,		SDL_SCANCODE_BACKSPACE },
	{ VK_TAB,		SDL_SCANCODE_TAB },
	{ VK_CLEAR,		SDL_SCANCODE_CLEAR },
	{ VK_RETURN,	SDL_SCANCODE_RETURN },
	{ VK_SHIFT,		SDL_SCANCODE_LSHIFT },
	{ VK_CONTROL,	SDL_SCANCODE_LCTRL },
	{ VK_MENU,		SDL_SCANCODE_LALT },
	{ VK_PAUSE,		SDL_SCANCODE_PAUSE },
	{ VK_CAPITAL,	SDL_SCANCODE_CAPSLOCK },
	{ VK_ESCAPE,	SDL_SCANCODE_ESCAPE },
	{ VK_SPACE,		SDL_SCANCODE_SPACE },
	{ VK_PRIOR,		SDL_SCANCODE_PAGEUP },
	{ VK_NEXT,		SDL_SCANCODE_PAGEDOWN },
	{ VK_END,		SDL_SCANCODE_END },
	{ VK_HOME,		SDL_SCANCODE_HOME },
	{ VK_LEFT,		SDL_SCANCODE_LEFT },
	{ VK_UP,		SDL_SCANCODE_UP },
	{ VK_RIGHT,		SDL_SCANCODE_RIGHT },
	{ VK_DOWN,		SDL_SCANCODE_DOWN },
	{ VK_SELECT,	SDL_SCANCODE_SELECT },
	{ VK_SNAPSHOT,	SDL_SCANCODE_PRINTSCREEN },
	{ VK_INSERT,	SDL_SCANCODE_INSERT },
	{ VK_DELETE,	SDL_SCANCODE_DELETE },
	{ VK_HELP,		SDL_SCANCODE_HELP },
	{ VK_NUMPAD0,	SDL_SCANCODE_KP_0 },
	{ VK_NUMPAD1,	SDL_SCANCODE_KP_1 },
	{ VK_NUMPAD2,	SDL_SCANCODE_KP_2 },
	{ VK_NUMPAD3,	SDL_SCANCODE_KP_3 },
	{ VK_NUMPAD4,	SDL_SCANCODE_KP_4 },
	{ VK_NUMPAD5,	SDL_SCANCODE_KP_5 },
	{ VK_NUMPAD6,	SDL_SCANCODE_KP_6 },
	{ VK_NUMPAD7,	SDL_SCANCODE_KP_7 },
	{ VK_NUMPAD8,	SDL_SCANCODE_KP_8 },
	{ VK_NUMPAD9,	SDL_SCANCODE_KP_9 },
	{ VK_MULTIPLY,	SDL_SCANCODE_KP_MULTIPLY },
	{ VK_ADD,		SDL_SCANCODE_KP_PLUS },
	{ VK_SEPARATOR,	SDL_SCANCODE_SEPARATOR },
	{ VK_SUBTRACT,	SDL_SCANCODE_KP_MINUS },
	{ VK_DECIMAL,	SDL_SCANCODE_KP_PERIOD },
	{ VK_DIVIDE,	SDL_SCANCODE_KP_DIVIDE },
	{ VK_F1,		SDL_SCANCODE_F1 },
	{ VK_F2,		SDL_SCANCODE_F2 },
	{ VK_F3,		SDL_SCANCODE_F3 },
	{ VK_F4,		SDL_SCANCODE_F4 },
	{ VK_F5,		SDL_SCANCODE_F5 },
	{ VK_F6,		SDL_SCANCODE_F6 },
	{ VK_F7,		SDL_SCANCODE_F7 },
	{ VK_F8,		SDL_SCANCODE_F8 },
	{ VK_F9,		SDL_SCANCODE_F9 },
	{ VK_F10,		SDL_SCANCODE_F10 },
	{ VK_F11,		SDL_SCANCODE_F11 },
	{ VK_F12,		SDL_SCANCODE_F12 },
	{ VK_F13,		SDL_SCANCODE_F13 },
	{ VK_F14,		SDL_SCANCODE_F14 },
	{ VK_F15,		SDL_SCANCODE_F15 },
	{ VK_F16,		SDL_SCANCODE_F16 },
	{ VK_F17,		SDL_SCANCODE_F17 },
	{ VK_F18,		SDL_SCANCODE_F18 },
	{ VK_F19,		SDL_SCANCODE_F19 },
	{ VK_F20,		SDL_SCANCODE_F20 },
	{ VK_F21,		SDL_SCANCODE_F21 },
	{ VK_F22,		SDL_SCANCODE_F22 },
	{ VK_F23,		SDL_SCANCODE_F23 },
	{ VK_F24,		SDL_SCANCODE_F24 },
	{ VK_NUMLOCK,	SDL_SCANCODE_NUMLOCKCLEAR },
	{ VK_SCROLL,	SDL_SCANCODE_SCROLLLOCK },
	{ VK_NONE_BA,	SDL_SCANCODE_SEMICOLON },
	{ VK_NONE_BB,	SDL_SCANCODE_EQUALS },
	{ VK_NONE_BC,	SDL_SCANCODE_COMMA },
	{ VK_NONE_BD,	SDL_SCANCODE_MINUS },
	{ VK_NONE_BE,	SDL_SCANCODE_PERIOD },
	{ VK_NONE_BF,	SDL_SCANCODE_SLASH },
	{ VK_NONE_C0,	SDL_SCANCODE_GRAVE },
	{ VK_NONE_DB,	SDL_SCANCODE_LEFTBRACKET },
	{ VK_NONE_DC,	SDL_SCANCODE_BACKSLASH },
	{ VK_NONE_DD,	SDL_SCANCODE_RIGHTBRACKET },
	{ VK_NONE_DE,	SDL_SCANCODE_APOSTROPHE },
};


SDL_Scancode VK_To_Scancode(unsigned short key)
{
	if (key >= VK_A && key <= VK_Z) return((SDL_Scancode)(SDL_SCANCODE_A + (key - VK_A)));

	for (KeyMapEntry const & entry : _KeyMap) {
		if (entry.Key == key) return(entry.Scancode);
	}
	return(SDL_SCANCODE_UNKNOWN);
}


unsigned short Scancode_To_VK(SDL_Scancode code)
{
	if (code >= SDL_SCANCODE_A && code <= SDL_SCANCODE_Z) return((unsigned short)(VK_A + (code - SDL_SCANCODE_A)));

	for (KeyMapEntry const & entry : _KeyMap) {
		if (entry.Scancode == code) return(entry.Key);
	}

	// The generic Shift/Control/Alt VK codes stand for either side of the keyboard; only
	// their left half is in the table above.
	switch (code) {
		case SDL_SCANCODE_RSHIFT:	return(VK_SHIFT);
		case SDL_SCANCODE_RCTRL:	return(VK_CONTROL);
		case SDL_SCANCODE_RALT:		return(VK_MENU);
		default:					return(VK_NONE);
	}
}


void Apply_Cursor_Visibility(void)
{
	if (_ShowCount < 0 || _ExplicitlyHidden) {
		SDL_HideCursor();
		return;
	}

	SDL_ShowCursor();
	SDL_SetCursor(_CurrentCursor != nullptr ? _CurrentCursor : SDL_GetDefaultCursor());
}


void Ensure_Video_Init(void)
{
	if (!SDL_WasInit(SDL_INIT_VIDEO)) {
		SDL_InitSubSystem(SDL_INIT_VIDEO);
	}
}

}	// namespace


Point2D Host_Pointer_Position(void)
{
	if (_Window == nullptr) return(Point2D(0, 0));

	float global_x, global_y;
	SDL_GetGlobalMouseState(&global_x, &global_y);

	int window_x, window_y;
	SDL_GetWindowPosition(_Window, &window_x, &window_y);

	return(Point2D((int)global_x - window_x, (int)global_y - window_y));
}


void Host_Move_Pointer(Point2D const & position)
{
	if (_Window == nullptr) return;

	SDL_WarpMouseInWindow(_Window, (float)position.X, (float)position.Y);
}


int Host_Show_Pointer(bool show)
{
	_ShowCount += show ? 1 : -1;
	Apply_Cursor_Visibility();
	return(_ShowCount);
}


void Host_Confine_Pointer(bool confine)
{
	_Confined = confine;
	if (_Window == nullptr) return;

	if (confine) {
		int width, height;
		SDL_GetWindowSize(_Window, &width, &height);
		SDL_Rect const rect{ 0, 0, width, height };
		SDL_SetWindowMouseRect(_Window, &rect);
	} else {
		SDL_SetWindowMouseRect(_Window, nullptr);
	}
}


void Host_Capture_Pointer(void)
{
	_Captured = true;
	SDL_CaptureMouse(true);
}


void Host_Release_Pointer(void)
{
	_Captured = false;
	SDL_CaptureMouse(false);
}


bool Host_Pointer_Is_Captured(void)
{
	return(_Captured);
}


// SDL has no portable system metric for it; 4 pixels matches the common desktop default
// Windows also ships (SM_CXDRAG/SM_CYDRAG).
Point2D Host_Drag_Threshold(void)
{
	return(Point2D(4, 4));
}


HostCursor * Host_Create_Cursor(std::uint32_t const * pixels, int width, int height, int hotx, int hoty)
{
	if (pixels == nullptr || width <= 0 || height <= 0) {
		return(nullptr);
	}

	SDL_Surface * surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_ARGB32,
		(void *)pixels, width * (int)sizeof(std::uint32_t));
	if (surface == nullptr) {
		return(nullptr);
	}

	SDL_Cursor * handle = SDL_CreateColorCursor(surface, hotx, hoty);
	SDL_DestroySurface(surface);

	if (handle == nullptr) {
		return(nullptr);
	}

	HostCursor * cursor = new HostCursor;
	cursor->Handle = handle;
	return(cursor);
}


void Host_Destroy_Cursor(HostCursor * cursor)
{
	if (cursor == nullptr) return;

	if (cursor->Handle == _CurrentCursor) {
		_CurrentCursor = nullptr;
		Apply_Cursor_Visibility();
	}

	SDL_DestroyCursor(cursor->Handle);
	delete cursor;
}


void Host_Set_Cursor(HostCursor * cursor)
{
	_CurrentCursor = (cursor != nullptr) ? cursor->Handle : nullptr;
	_ExplicitlyHidden = false;
	Apply_Cursor_Visibility();
}


void Host_Hide_Cursor(void)
{
	_ExplicitlyHidden = true;
	Apply_Cursor_Visibility();
}


bool Host_Key_Is_Down(unsigned short key)
{
	if (key == VK_LBUTTON || key == VK_RBUTTON || key == VK_MBUTTON) {
		SDL_MouseButtonFlags const buttons = SDL_GetMouseState(nullptr, nullptr);

		switch (key) {
			case VK_LBUTTON: return((buttons & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) != 0);
			case VK_MBUTTON: return((buttons & SDL_BUTTON_MASK(SDL_BUTTON_MIDDLE)) != 0);
			case VK_RBUTTON: return((buttons & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT)) != 0);
		}
		return(false);
	}

	bool const * state = SDL_GetKeyboardState(nullptr);

	// The generic Shift/Control/Alt VK codes stand for either side of the keyboard.
	switch (key) {
		case VK_SHIFT:		return(state[SDL_SCANCODE_LSHIFT] || state[SDL_SCANCODE_RSHIFT]);
		case VK_CONTROL:	return(state[SDL_SCANCODE_LCTRL] || state[SDL_SCANCODE_RCTRL]);
		case VK_MENU:		return(state[SDL_SCANCODE_LALT] || state[SDL_SCANCODE_RALT]);
	}

	SDL_Scancode const code = VK_To_Scancode(key & 0xFF);
	return(code != SDL_SCANCODE_UNKNOWN && state[code]);
}


unsigned short Host_Key_Modifiers(void)
{
	SDL_Keymod const mod = SDL_GetModState();
	unsigned short modifiers = 0;

	if (mod & SDL_KMOD_SHIFT) modifiers |= WWKEY_SHIFT_BIT;
	if (mod & SDL_KMOD_CTRL) modifiers |= WWKEY_CTRL_BIT;
	if (mod & SDL_KMOD_ALT) modifiers |= WWKEY_ALT_BIT;

	return(modifiers);
}


// Translates with the modifiers the key code carries, as the Win32 host does, rather than
// those held now.
int Host_Key_To_Character(unsigned short key)
{
	SDL_Scancode const code = VK_To_Scancode(key & 0xFF);
	if (code == SDL_SCANCODE_UNKNOWN) return(0);

	Uint16 mod = 0;
	if (key & WWKEY_SHIFT_BIT) mod |= SDL_KMOD_SHIFT;
	if (key & WWKEY_CTRL_BIT) mod |= SDL_KMOD_CTRL;
	if (key & WWKEY_ALT_BIT) mod |= SDL_KMOD_ALT;

	// SDL_Keycode values for a text-producing key are the character's own code point; a key
	// that produces no text carries the scancode instead, flagged with SDLK_SCANCODE_MASK.
	SDL_Keycode const keycode = SDL_GetKeyFromScancode(code, (SDL_Keymod)mod, true);
	if (keycode == SDLK_UNKNOWN || (keycode & SDLK_SCANCODE_MASK) != 0) return(0);

	return((int)keycode);
}


bool Has_Main_Window(void)
{
	return(_Window != nullptr);
}


void Host_Create_Window(int width, int height)
{
	Ensure_Video_Init();

	int clientwidth = width;
	int clientheight = height;
	SDL_WindowFlags flags = 0;

	if (WindowedMode) {
		if (Options.WindowWidth > 0) clientwidth = Options.WindowWidth;
		if (Options.WindowHeight > 0) clientheight = Options.WindowHeight;
	} else {
		// "Fullscreen window at desktop resolution": the current desktop mode, borderless,
		// matching the Win32 host's WS_POPUP window sized to the screen.
		flags |= SDL_WINDOW_FULLSCREEN;

		SDL_Rect bounds;
		if (SDL_GetDisplayBounds(SDL_GetPrimaryDisplay(), &bounds)) {
			clientwidth = bounds.w;
			clientheight = bounds.h;
		}
	}

	_Window = SDL_CreateWindow("Tiberian Sun", std::max(clientwidth, 1), std::max(clientheight, 1), flags);
	if (_Window == nullptr) return;

	if (WindowedMode) {
		SDL_SetWindowPosition(_Window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	}

	SDL_GetWindowSizeInPixels(_Window, &_LastWidth, &_LastHeight);

	Game_Window_Created();
}


void Host_Close_Window(void)
{
	if (_Window == nullptr) return;

	SDL_DestroyWindow(_Window);
	_Window = nullptr;

	Game_Window_Destroyed();
}


// bgfx's Default platform type is the X11 pairing hostwindow_win32.cpp's HWND takes for
// Windows; SDL reports the same Display*/Window pair under X11 (Xwayland included), and the
// wl_display/wl_surface pair when it is really running against Wayland. nativewindow.hh
// already anticipates both.
NativeWindow Host_Native_Window(void)
{
	if (_Window == nullptr) return(NativeWindow{ NATIVE_WINDOW_DEFAULT, nullptr, nullptr });

	SDL_PropertiesID const props = SDL_GetWindowProperties(_Window);

	void * wayland_display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
	if (wayland_display != nullptr) {
		void * wayland_surface = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
		return(NativeWindow{ NATIVE_WINDOW_WAYLAND, wayland_display, wayland_surface });
	}

	void * x11_display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
	Sint64 const x11_window = SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
	return(NativeWindow{ NATIVE_WINDOW_DEFAULT, x11_display, (void *)(std::uintptr_t)x11_window });
}


bool Host_Window_Drawable_Size(int & width, int & height)
{
	if (_Window == nullptr) return(false);
	if (!SDL_GetWindowSizeInPixels(_Window, &width, &height)) return(false);
	return(width > 0 && height > 0);
}


int Host_Window_Refresh_Rate(void)
{
	if (_Window == nullptr) return(0);

	SDL_DisplayID const display = SDL_GetDisplayForWindow(_Window);
	SDL_DisplayMode const * mode = SDL_GetCurrentDisplayMode(display);
	return(mode != nullptr ? (int)(mode->refresh_rate + 0.5f) : 0);
}


// A software Expose/InvalidateRect has no direct SDL equivalent for an accelerated window,
// so the request is folded into the same exposed-event path Host_Pump_Events already answers.
void Host_Invalidate_Window(void)
{
	if (_Window == nullptr) return;

	SDL_Event event;
	SDL_zero(event);
	event.type = SDL_EVENT_WINDOW_EXPOSED;
	event.window.windowID = SDL_GetWindowID(_Window);
	SDL_PushEvent(&event);
}


void Host_Focus_Window(void)
{
	if (_Window == nullptr) return;

	SDL_RaiseWindow(_Window);
}


void Host_Fit_Window_To_Frame(int width, int height)
{
	if (_Window == nullptr) return;

	int current_width, current_height;
	SDL_GetWindowSize(_Window, &current_width, &current_height);

	int x, y;
	SDL_GetWindowPosition(_Window, &x, &y);
	x += (current_width - width) / 2;
	y += (current_height - height) / 2;

	// Growing about the middle can push the window past the edges of the display, and a
	// title bar above the top of it cannot be grabbed to bring the window back.
	SDL_Rect work;
	if (SDL_GetDisplayUsableBounds(SDL_GetDisplayForWindow(_Window), &work)) {
		if (x + width > work.x + work.w) x = work.x + work.w - width;
		if (y + height > work.y + work.h) y = work.y + work.h - height;
		if (x < work.x) x = work.x;
		if (y < work.y) y = work.y;
	}

	SDL_SetWindowPosition(_Window, x, y);
	SDL_SetWindowSize(_Window, std::max(width, 1), std::max(height, 1));
}


bool Host_Display_Mode(int index, int & width, int & height)
{
	// Fetched once and kept for the process's life: the list rarely changes and
	// SDL_GetFullscreenDisplayModes hands back an allocation this host would otherwise have
	// to free and re-fetch on every call.
	static SDL_DisplayMode ** modes = nullptr;
	static int count = 0;
	static bool fetched = false;

	if (!fetched) {
		fetched = true;
		Ensure_Video_Init();
		modes = SDL_GetFullscreenDisplayModes(SDL_GetPrimaryDisplay(), &count);
	}

	if (modes == nullptr || index < 0 || index >= count) return(false);

	width = modes[index]->w;
	height = modes[index]->h;
	return(true);
}


// This host asks the player through SDL's own message box, which draws with X11 primitives
// directly rather than through a formal toolkit; a host with no video target at all fails
// the call, and the answer then falls back to hostwindow.h's documented dismissed-box
// default.
HostMessageBoxAnswer Host_Message_Box(char const * caption, char const * text, unsigned int style)
{
	bool const yes_no = (style & HOST_BOX_YES_NO) != 0;

	SDL_MessageBoxFlags flags = 0;
	switch (style & (HOST_BOX_ERROR | HOST_BOX_QUESTION | HOST_BOX_WARNING)) {
		case HOST_BOX_WARNING:		flags = SDL_MESSAGEBOX_WARNING; break;
		case HOST_BOX_ERROR:		flags = SDL_MESSAGEBOX_ERROR; break;
		case HOST_BOX_QUESTION:		flags = SDL_MESSAGEBOX_INFORMATION; break;
		default:					flags = SDL_MESSAGEBOX_INFORMATION; break;
	}

	SDL_MessageBoxButtonData const yes_no_buttons[] = {
		{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, HOST_ANSWER_NO, "No" },
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, HOST_ANSWER_YES, "Yes" },
	};
	SDL_MessageBoxButtonData const ok_button[] = {
		{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT | SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, HOST_ANSWER_OK, "OK" },
	};

	SDL_MessageBoxData data;
	SDL_zero(data);
	data.flags = flags;
	data.window = _Window;
	data.title = caption;
	data.message = text;
	data.numbuttons = yes_no ? 2 : 1;
	data.buttons = yes_no ? yes_no_buttons : ok_button;

	int buttonid = yes_no ? (int)HOST_ANSWER_NO : (int)HOST_ANSWER_OK;
	if (!SDL_ShowMessageBox(&data, &buttonid)) {
		return(yes_no ? HOST_ANSWER_NO : HOST_ANSWER_OK);
	}

	return((HostMessageBoxAnswer)buttonid);
}


void Host_Pump_Events(void)
{
	if (_Window == nullptr) return;

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
				// Mirrors the SC_CLOSE handling the Win32 window procedure gives its title
				// bar close box: a running game resigns instead of vanishing.
				if (GameActive && PlayerPtr != NULL && !Session.Play) {
					Queue_Exit();
				} else {
					Host_Close_Window();
					return;
				}
				break;

			case SDL_EVENT_WINDOW_RESIZED: {
				int new_width, new_height;
				if (SDL_GetWindowSizeInPixels(_Window, &new_width, &new_height)
						&& (new_width != _LastWidth || new_height != _LastHeight)) {
					_LastWidth = new_width;
					_LastHeight = new_height;
					Video_On_Resize(_LastWidth, _LastHeight);
					Video_Set_Refresh_Rate(Host_Window_Refresh_Rate());
				}
				break;
			}

			case SDL_EVENT_WINDOW_EXPOSED:
				Game_Window_On_Paint(GameInFocus == true || WindowedMode == true);
				break;

			case SDL_EVENT_WINDOW_FOCUS_GAINED:
				if (!GameInFocus) {
					GameInFocus = true;
					Focus_Restore();
				}
				break;

			case SDL_EVENT_WINDOW_FOCUS_LOST:
				if (GameInFocus) {
					GameInFocus = false;
					Focus_Loss();
				}
				break;

			case SDL_EVENT_KEY_DOWN:
			case SDL_EVENT_KEY_UP:
				// Scroll Lock was a debugger's breakpoint key and types nothing. A key SDL
				// repeats while held is taken only once, on its first press.
				if (event.key.scancode == SDL_SCANCODE_SCROLLLOCK && event.type == SDL_EVENT_KEY_DOWN) {
					break;
				}
				if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat) {
					break;
				}
				if (Keyboard != nullptr) {
					unsigned short const vk = Scancode_To_VK(event.key.scancode);
					if (vk != VK_NONE) {
						Keyboard->Post_Key_Event(vk, event.type == SDL_EVENT_KEY_UP);
					}
				}
				break;

			case SDL_EVENT_MOUSE_BUTTON_DOWN:
			case SDL_EVENT_MOUSE_BUTTON_UP: {
				unsigned short vk = VK_NONE;
				switch (event.button.button) {
					case SDL_BUTTON_LEFT:		vk = VK_LBUTTON; break;
					case SDL_BUTTON_MIDDLE:		vk = VK_MBUTTON; break;
					case SDL_BUTTON_RIGHT:		vk = VK_RBUTTON; break;
				}

				if (vk != VK_NONE) {
					Point2D point((int)event.button.x, (int)event.button.y);
					Window_Point_To_Game(point);

					if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
						Game_Window_Mouse_Button(vk, point, true);
					} else if (event.button.clicks >= 2) {
						Game_Window_Mouse_Double_Click(vk, point);
					} else {
						Game_Window_Mouse_Button(vk, point, false);
					}
				}
				break;
			}

			case SDL_EVENT_MOUSE_WHEEL:
				Game_Window_On_Mouse_Wheel(event.wheel.y > 0.0f ? 120 : -120);
				break;

			default:
				break;
		}
	}
}

#endif	// __linux__
