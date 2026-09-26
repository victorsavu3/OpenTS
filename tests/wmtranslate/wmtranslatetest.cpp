/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

// Pins the seam Windows_Message_Handler (sdlstub.cpp) translates SDL events across: a mouse
// move or click, a wheel step, and a key press all have to reach Map/UIShell/Keyboard as the
// same (message, wParam, lParam) triple winstub.cpp's own WM_* handlers already expect. Needs
// no game data.

#include "always.h"
#include "keyboard.h"
#include "win.h"
#include "winstub.h"

#include <cstdio>

namespace {

int Failures = 0;

void Check(bool condition, char const * what)
{
	std::printf("%-64s %s\n", what, condition ? "ok" : "FAILED");

	if (!condition) {
		Failures++;
	}
}

}	// namespace


int main(void)
{
	// A mouse move carries its position as the signed x/y pair WM_MOUSEMOVE's lParam always
	// has, whichever handler reads it back out with GET_X_LPARAM/GET_Y_LPARAM.
	LPARAM move = MAKELPARAM((short)640, (short)360);
	Check(GET_X_LPARAM(move) == 640 && GET_Y_LPARAM(move) == 360,
		"a mouse move's position round-trips through MAKELPARAM/GET_X_LPARAM/GET_Y_LPARAM");

	LPARAM negative = MAKELPARAM((short)-5, (short)-5);
	Check(GET_X_LPARAM(negative) == -5 && GET_Y_LPARAM(negative) == -5,
		"a position off the top or left edge stays negative, not a large unsigned coordinate");

	// A click carries the same position, on the message its button and transition name.
	Check(Mouse_Button_Message(SDL_BUTTON_LEFT, true) == WM_LBUTTONDOWN, "a left button down is WM_LBUTTONDOWN");
	Check(Mouse_Button_Message(SDL_BUTTON_LEFT, false) == WM_LBUTTONUP, "a left button up is WM_LBUTTONUP");
	Check(Mouse_Button_Message(SDL_BUTTON_RIGHT, true) == WM_RBUTTONDOWN, "a right button down is WM_RBUTTONDOWN");
	Check(Mouse_Button_Message(SDL_BUTTON_MIDDLE, true) == WM_MBUTTONDOWN, "a middle button down is WM_MBUTTONDOWN");

	// A step of the wheel is carried in wParam's high word, in the units WHEEL_DELTA already
	// names, and GET_WHEEL_DELTA_WPARAM has to read the same signed value back out.
	int const one_step_up = (int)(1.0f * 120.0f);
	WPARAM wheel_up = MAKELONG(0, one_step_up);
	Check(one_step_up == WHEEL_DELTA, "one wheel step is WHEEL_DELTA");
	Check(GET_WHEEL_DELTA_WPARAM(wheel_up) == WHEEL_DELTA, "a step up reads back as a positive WHEEL_DELTA");

	int const one_step_down = (int)(-1.0f * 120.0f);
	WPARAM wheel_down = MAKELONG(0, one_step_down);
	Check(GET_WHEEL_DELTA_WPARAM(wheel_down) == -WHEEL_DELTA, "a step down reads back as a negative WHEEL_DELTA");

	// A key press names the same physical key both directions of this build's fixed layout
	// agree on.
	Check(Scancode_To_VK(SDL_SCANCODE_A) == VK_A, "the A key's scancode names VK_A");
	Check(Scancode_To_VK(SDL_SCANCODE_RETURN) == VK_RETURN, "the Return key's scancode names VK_RETURN");
	Check(Scancode_To_VK(SDL_SCANCODE_LEFT) == VK_LEFT, "the Left arrow's scancode names VK_LEFT");
	Check(VK_To_Scancode(VK_A) == SDL_SCANCODE_A, "VK_A names the A key's scancode back");
	Check(VK_To_Scancode(VK_RETURN) == SDL_SCANCODE_RETURN, "VK_RETURN names the Return key's scancode back");

	// A scancode or VK this build's fixed layout does not carry maps to the table's own
	// documented empty entry, not a stale or garbage code.
	Check(Scancode_To_VK(SDL_SCANCODE_UNKNOWN) == VK_NONE, "an unmapped scancode names no VK");
	Check(VK_To_Scancode(VK_NONE) == SDL_SCANCODE_UNKNOWN, "an unmapped VK names no scancode");

	std::printf("\n%s\n", Failures == 0 ? "All checks passed." : "There were failures.");
	return(Failures == 0 ? 0 : 1);
}
