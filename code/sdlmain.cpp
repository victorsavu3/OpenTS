/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

// The process entry point, kept apart from sdlstub.cpp so a harness can link that file's
// translation helpers without pulling in a second main().

#include "always.h"

#include "win.h"
#include "winstub.h"

namespace {
int ProgramArgc = 0;
char ** ProgramArgv = nullptr;
} // namespace


char * const * Program_Arguments(int & count)
{
	count = ProgramArgc;
	return(ProgramArgv);
}


int CALLBACK WinMain(HINSTANCE, HINSTANCE, char *, int);

// WinMain never receives argc/argv on Windows either, so it always fetches them itself;
// on Linux the ones main() already has are simply handed to it through Program_Arguments.
int main(int argc, char ** argv)
{
	ProgramArgc = argc;
	ProgramArgv = argv;
	return(WinMain((HINSTANCE)1, nullptr, "", SW_NORMAL));
}
