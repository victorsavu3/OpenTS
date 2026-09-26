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

#pragma once

#include "win.h"

class Surface;
class PaletteClass;
struct NativeWindow;

void Create_Main_Window ( HINSTANCE instance , int command_show , int width , int height);
NativeWindow Win_Native_Window(HWND window);
bool Win_Window_Drawable_Size(HWND window, int & width, int & height);
int Win_Window_Refresh_Rate(HWND window);
#ifndef _WIN32
#include <SDL3/SDL.h>

void Win_Resize_And_Center_Window(HWND window, int width, int height);

// The real argv main() received, so WinMain's own callers need no GetCommandLineW-style
// reconstruction. count is the number of entries; argv[0] is the executable path.
char * const * Program_Arguments(int & count);

// Translates an SDL_EVENT_MOUSE_BUTTON_DOWN/UP event's button into the matching
// WM_*BUTTONDOWN/UP message.
UINT Mouse_Button_Message(Uint8 button, bool down);

// The two directions of this build's fixed SDL_Scancode <-> VK_* mapping. A scancode or VK
// outside the table maps to SDL_SCANCODE_UNKNOWN or 0, respectively.
unsigned short Scancode_To_VK(SDL_Scancode scancode);
SDL_Scancode VK_To_Scancode(unsigned short vk);
#endif

void Load_Title_Screen(char const * name, Surface * surface, PaletteClass * palette);

unsigned int Build_Number(void);
