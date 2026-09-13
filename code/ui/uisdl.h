/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

// Only the Linux host reaches SDL; this header, like code/hostwindow_sdl.cpp, compiles to
// nothing elsewhere.
#if defined(__linux__)

#include <SDL3/SDL.h>


// Offers an SDL event to the UI shell, returning true when a document consumed it. This is
// the whole of the shell's dependence on SDL: it translates events into the shell's own
// input events, so retiring SDL here costs this file and nothing behind it.
//
// It belongs in Host_Pump_Events after a mouse position has been taken into the frame, and
// before the game's own input handling, which keeps consumed input out of the KN_ queue, the
// same order code/hostwindow_win32.cpp calls UI_Handle_Window_Message in.
bool UI_Handle_SDL_Event(SDL_Event const & event);

// code/hostwindow_sdl.cpp owns the VK_ table this key belongs to; UI_Handle_SDL_Event reads a
// key event through it rather than keeping a second table of its own.
unsigned short SDL_Scancode_To_VK(SDL_Scancode code);

#endif	// __linux__
