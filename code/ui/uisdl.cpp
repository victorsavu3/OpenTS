/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

// The window system's side of the UI shell's input, over SDL. code/ui/uiwin32.cpp is the
// same file for Win32; a position has already been taken into the frame by the time either
// sees one, and code/hostwindow_sdl.cpp owns the VK_ table SDL_Scancode_To_VK reads.

#if defined(__linux__)

#include "always.h"

#include "uisdl.h"

#include "hostwindow.h"
#include "uikeymap.h"
#include "uishell.h"


static unsigned int Current_Modifiers(void)
{
	SDL_Keymod const mod = SDL_GetModState();
	unsigned int modifiers = UI_MODIFIER_NONE;

	if (mod & SDL_KMOD_SHIFT) modifiers |= UI_MODIFIER_SHIFT;
	if (mod & SDL_KMOD_CTRL) modifiers |= UI_MODIFIER_CONTROL;
	if (mod & SDL_KMOD_ALT) modifiers |= UI_MODIFIER_ALT;
	if (mod & SDL_KMOD_GUI) modifiers |= UI_MODIFIER_META;

	return(modifiers);
}


// A press that a document took owns its release, so the window keeps the mouse until the
// button comes back up even if the cursor leaves the frame in between.
static bool _Captured = false;


static bool Map_Button(Uint8 button, UIMouseButtonType & mapped)
{
	switch (button) {
		case SDL_BUTTON_LEFT:		mapped = UI_MOUSE_LEFT;	return(true);
		case SDL_BUTTON_RIGHT:		mapped = UI_MOUSE_RIGHT;	return(true);
		case SDL_BUTTON_MIDDLE:	mapped = UI_MOUSE_MIDDLE;	return(true);
		default:					return(false);
	}
}


static bool Handle_Button(UIMouseButtonType button, bool down, int x, int y)
{
	if (!down && _Captured) {
		_Captured = false;
		Host_Release_Pointer();

		// The owner of the press owns the release whatever the document now reports, so
		// the release is delivered and consumed either way.
		UI_Handle_Mouse_Button(button, false, x, y, Current_Modifiers());
		return(true);
	}

	if (!UI_Handle_Mouse_Button(button, down, x, y, Current_Modifiers())) {
		return(false);
	}

	if (down) {
		_Captured = true;
		Host_Capture_Pointer();
	}

	return(true);
}


bool UI_Handle_SDL_Event(SDL_Event const & event)
{
	if (!UI_Is_Initialized()) {
		return(false);
	}

	switch (event.type) {
		case SDL_EVENT_MOUSE_MOTION:
			// A move is never consumed: the game goes on tracking the cursor whatever a
			// document is doing with it.
			UI_Handle_Mouse_Move((int)event.motion.x, (int)event.motion.y, Current_Modifiers());
			return(false);

		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_MOUSE_BUTTON_UP: {
			UIMouseButtonType button;
			if (!Map_Button(event.button.button, button)) {
				return(false);
			}
			return(Handle_Button(button, event.type == SDL_EVENT_MOUSE_BUTTON_DOWN,
				(int)event.button.x, (int)event.button.y));
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

			return(UI_Handle_Mouse_Wheel(lines, Current_Modifiers()));
		}

		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP:
			return(UI_Handle_Key(UI_Key_From_Virtual(SDL_Scancode_To_VK(event.key.scancode)),
				event.type == SDL_EVENT_KEY_DOWN, Current_Modifiers()));

		case SDL_EVENT_TEXT_INPUT:
			return(UI_Handle_Text(event.text.text));

		case SDL_EVENT_WINDOW_FOCUS_LOST:
			if (_Captured) {
				_Captured = false;
				Host_Release_Pointer();
			}
			UI_On_Focus_Lost();
			return(false);

		default:
			return(false);
	}
}

#endif	// __linux__
