/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "ui/uiunicode.h"

#include "utf8.h"

#include <climits>
#include <new>

#if defined(_WIN32)
#include <windows.h>
#endif


#if defined(_WIN32)

bool UI_UTF8_To_UTF16(std::string_view text, std::wstring & wide)
{
	wide.clear();

	if (text.size() > UI_CLIPBOARD_MAX_BYTES || text.size() > INT_MAX) {
		return(false);
	}
	if (text.empty()) {
		return(true);
	}

	int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), (int)text.size(), nullptr, 0);
	if (length == 0) {
		return(false);
	}

	try {
		wide.resize((std::size_t)length);
	} catch (std::bad_alloc const &) {
		return(false);
	}

	if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), (int)text.size(), wide.data(), length) != length) {
		wide.clear();
		return(false);
	}
	return(true);
}


bool UI_UTF16_To_UTF8(std::wstring_view wide, std::string & text)
{
	text.clear();

	if (wide.size() > UI_CLIPBOARD_MAX_BYTES / sizeof(wchar_t) || wide.size() > INT_MAX) {
		return(false);
	}
	if (wide.empty()) {
		return(true);
	}

	int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(), (int)wide.size(), nullptr, 0, nullptr, nullptr);
	if (length == 0) {
		return(false);
	}

	try {
		text.resize((std::size_t)length);
	} catch (std::bad_alloc const &) {
		return(false);
	}

	if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(), (int)wide.size(), text.data(), length, nullptr, nullptr) != length) {
		text.clear();
		return(false);
	}
	return(true);
}

#else	// _WIN32

// Off Windows there is no CF_UNICODETEXT to feed, only the code points themselves, so this
// packs and unpacks them as UTF-16 code units by numeric value regardless of the platform's
// own (wider) wchar_t: each element still holds one code unit, surrogate pairs included.
bool UI_UTF8_To_UTF16(std::string_view text, std::wstring & wide)
{
	wide.clear();

	if (text.size() > UI_CLIPBOARD_MAX_BYTES || !UTF8::Is_Valid(text)) {
		return(false);
	}

	try {
		char const * cursor = text.data();
		char const * const end = cursor + text.size();
		while (cursor < end) {
			char32_t code = UTF8::Decode(cursor);
			if (code < 0x10000) {
				wide.push_back((wchar_t)code);
			} else {
				code -= 0x10000;
				wide.push_back((wchar_t)(0xD800 + (code >> 10)));
				wide.push_back((wchar_t)(0xDC00 + (code & 0x3FF)));
			}
		}
	} catch (std::bad_alloc const &) {
		wide.clear();
		return(false);
	}
	return(true);
}


bool UI_UTF16_To_UTF8(std::wstring_view wide, std::string & text)
{
	text.clear();

	if (wide.size() > UI_CLIPBOARD_MAX_BYTES / 2) {
		return(false);
	}

	try {
		for (std::size_t index = 0; index < wide.size(); index++) {
			char32_t unit = (char32_t)(unsigned short)wide[index];
			char32_t code;

			if (unit >= 0xD800 && unit <= 0xDBFF) {
				if (index + 1 >= wide.size()) {
					text.clear();
					return(false);
				}
				char32_t low = (char32_t)(unsigned short)wide[index + 1];
				if (low < 0xDC00 || low > 0xDFFF) {
					text.clear();
					return(false);
				}
				code = 0x10000 + ((unit - 0xD800) << 10) + (low - 0xDC00);
				index++;
			} else if (unit >= 0xDC00 && unit <= 0xDFFF) {
				text.clear();
				return(false);
			} else {
				code = unit;
			}

			char buffer[UTF8::MAX_SEQUENCE];
			text.append(buffer, UTF8::Encode(code, buffer));
		}
	} catch (std::bad_alloc const &) {
		text.clear();
		return(false);
	}
	return(true);
}

#endif	// _WIN32
