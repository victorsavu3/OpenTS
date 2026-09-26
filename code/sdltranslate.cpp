/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

// The SDL event -> Win32 message translation seam sdlstub.cpp's poll loop drives, kept in its
// own file, with no other engine dependency, so a test harness can link it alone.

#include "always.h"

#include "keyboard.h"
#include "win.h"
#include "winstub.h"


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
