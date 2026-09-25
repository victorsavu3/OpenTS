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
void Win_Resize_And_Center_Window(HWND window, int width, int height);

// The real argv main() received, so WinMain's own callers need no GetCommandLineW-style
// reconstruction. count is the number of entries; argv[0] is the executable path.
char * const * Program_Arguments(int & count);
#endif

void Load_Title_Screen(char const * name, Surface * surface, PaletteClass * palette);

unsigned int Build_Number(void);
