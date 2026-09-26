/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

// Pins the millisecond timing contract every platform's timeGetTime/GetTickCount64/
// QueryPerformanceCounter shares: what unit each reports, that none of them runs backward, and
// that a duration computed across the 32-bit counter's wraparound still comes out right. Needs
// no game data.

#include "always.h"
#include "win.h"

#include <chrono>
#include <cstdio>
#include <thread>

namespace {

int Failures = 0;

void Check(bool condition, char const * what)
{
	std::printf("%-64s %s\n", what, condition ? "ok" : "FAILED");

	if (!condition) {
		Failures++;
	}
}

void Sleep_Ms(int milliseconds)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

}	// namespace


int main(void)
{
	DWORD first = timeGetTime();
	DWORD last = first;
	bool ever_went_backward = false;
	for (int sample = 0; sample < 1000; sample++) {
		DWORD next = timeGetTime();
		if (next != last && (DWORD)(next - last) > 0x80000000u) {
			ever_went_backward = true;
		}
		last = next;
	}
	Check(!ever_went_backward, "timeGetTime never runs backward across repeated calls");

	DWORD before = timeGetTime();
	Sleep_Ms(50);
	DWORD after = timeGetTime();
	DWORD elapsed_ms = after - before;
	Check(elapsed_ms >= 40 && elapsed_ms <= 2000, "timeGetTime reports milliseconds, not another unit");

	LARGE_INTEGER frequency;
	Check(QueryPerformanceFrequency(&frequency) && frequency.QuadPart > 0,
		"QueryPerformanceFrequency reports a positive tick rate");

	LARGE_INTEGER qpc_before;
	QueryPerformanceCounter(&qpc_before);
	Sleep_Ms(50);
	LARGE_INTEGER qpc_after;
	QueryPerformanceCounter(&qpc_after);

	double seconds = (double)(qpc_after.QuadPart - qpc_before.QuadPart) / (double)frequency.QuadPart;
	Check(seconds >= 0.03 && seconds <= 2.0,
		"QueryPerformanceCounter divided by QueryPerformanceFrequency reports elapsed seconds");

	// The 32-bit counter wraps every so often; a caller's own (DWORD)(later - earlier) still
	// gives the right duration across that wraparound, since unsigned subtraction is modular.
	DWORD earlier = 0xFFFFFFF0u;
	DWORD later = 0x00000010u;
	Check((DWORD)(later - earlier) == 0x20u,
		"a duration spanning the counter's wraparound still comes out correct");

	// GetTickCount64 exists precisely so a caller does not have to reason about that
	// wraparound at all; it has to actually be 64 bits wide to do that.
	Check(sizeof(GetTickCount64()) == 8, "GetTickCount64 is a 64-bit count, unlike timeGetTime");

	// The two calls are not atomic with each other, so either one can read a touch ahead of the
	// other; the gap is scheduling jitter, not something this test controls, so the bound is
	// generous rather than tight. Casting the unsigned difference back to signed recovers a small
	// negative gap correctly instead of reading it as wrapped around to nearly 2^32.
	ULONGLONG wide_before = GetTickCount64();
	DWORD narrow_before = timeGetTime();
	LONG difference_ms = (LONG)(narrow_before - (DWORD)wide_before);
	Check(difference_ms >= -1000 && difference_ms <= 1000,
		"GetTickCount64 and timeGetTime agree, save for the instant that passes between the two calls");

	std::printf("\n%s\n", Failures == 0 ? "All checks passed." : "There were failures.");
	return(Failures == 0 ? 0 : 1);
}
