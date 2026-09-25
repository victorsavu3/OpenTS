/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

// The window system's side of the UI shell's input, over SDL. It translates each SDL event
// into the window message UIShellClass::Handle_Window_Message answers on Win32, which keeps
// one input contract for the shell regardless of host; code/hostwindow_win32.cpp calls that
// entry point directly from its own window procedure, with no adapter file of its own.

#if defined(__linux__)

#include "always.h"

#include "uisdl.h"

#include "_ui.h"
#include "keyboard.h"
#include "ui/uishell.h"
#include "ui/uiunicode.h"

#include <string>


static LPARAM Point_LParam(float x, float y)
{
	return(MAKELPARAM((int)x, (int)y));
}


bool UI_Handle_SDL_Event(SDL_Event const & event)
{
	switch (event.type) {
		case SDL_EVENT_MOUSE_MOTION:
			// A move is never consumed: the game goes on tracking the cursor whatever a
			// document is doing with it.
			UIShell.Handle_Window_Message(nullptr, WM_MOUSEMOVE, 0, Point_LParam(event.motion.x, event.motion.y));
			return(false);

		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_MOUSE_BUTTON_UP: {
			bool const down = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
			UINT message;
			switch (event.button.button) {
				case SDL_BUTTON_LEFT:
					message = !down ? WM_LBUTTONUP : (event.button.clicks >= 2 ? WM_LBUTTONDBLCLK : WM_LBUTTONDOWN);
					break;
				case SDL_BUTTON_MIDDLE:
					message = !down ? WM_MBUTTONUP : (event.button.clicks >= 2 ? WM_MBUTTONDBLCLK : WM_MBUTTONDOWN);
					break;
				case SDL_BUTTON_RIGHT:
					message = !down ? WM_RBUTTONUP : (event.button.clicks >= 2 ? WM_RBUTTONDBLCLK : WM_RBUTTONDOWN);
					break;
				default:
					return(false);
			}
			return(UIShell.Handle_Window_Message(nullptr, message, 0, Point_LParam(event.button.x, event.button.y)));
		}

		case SDL_EVENT_MOUSE_WHEEL: {
			// RmlUi scrolls in lines and reads a positive delta as downward, the opposite
			// of SDL's own sign (positive is away from the player, i.e. up, unless the
			// platform reports the axes flipped); a delta that rounds to no lines at all
			// still scrolls the way it points.
			float lines = (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) ? event.wheel.y : -event.wheel.y;
			if (lines > -1.0f && lines < 0.0f) {
				lines = -1.0f;
			} else if (lines > 0.0f && lines < 1.0f) {
				lines = 1.0f;
			}

			WPARAM const wparam = MAKEWPARAM(0, (unsigned short)(short)(lines * WHEEL_DELTA));
			return(UIShell.Handle_Window_Message(nullptr, WM_MOUSEWHEEL, wparam, Point_LParam(event.wheel.mouse_x, event.wheel.mouse_y)));
		}

		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP: {
			unsigned short const vk = SDL_Scancode_To_VK(event.key.scancode);
			if (vk == VK_NONE) {
				return(false);
			}

			bool const down = (event.type == SDL_EVENT_KEY_DOWN);
			LPARAM const lparam = (down && event.key.repeat) ? (1 << 30) : 0;
			return(UIShell.Handle_Window_Message(nullptr, down ? WM_KEYDOWN : WM_KEYUP, vk, lparam));
		}

		case SDL_EVENT_TEXT_INPUT: {
			std::wstring wide;
			if (!UI_UTF8_To_UTF16(event.text.text, wide)) {
				return(false);
			}

			bool consumed = false;
			for (wchar_t unit : wide) {
				consumed = UIShell.Handle_Window_Message(nullptr, WM_CHAR, (WPARAM)unit, 0) || consumed;
			}
			return(consumed);
		}

		case SDL_EVENT_WINDOW_FOCUS_LOST:
			// Drops the shell's own capture and composition state; never consumed, the
			// same way a real WM_ACTIVATEAPP answer never is.
			UIShell.Handle_Window_Message(nullptr, WM_ACTIVATEAPP, 0, 0);
			return(false);

		case SDL_EVENT_WINDOW_FOCUS_GAINED:
			UIShell.Handle_Window_Message(nullptr, WM_ACTIVATEAPP, 1, 0);
			return(false);

		default:
			return(false);
	}
}

#endif	// __linux__
