/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

// Pins the generated language string table (cmake/LanguageStrings.cmake) against the text
// language.rc actually ships, and Fetch_String's non-Windows path against that same table.
// Needs no game data: the strings it checks are read from the repository's own language.rc at
// build time, not from anything the player installs.

#include "always.h"

#include "data.h"
#include "language/language.h"
#include "opents_languagestrings.h"

#include <cstdio>
#include <cstring>

namespace {

int Failures = 0;

void Check(bool condition, char const * what)
{
	std::printf("%-64s %s\n", what, condition ? "ok" : "FAILED");

	if (!condition) {
		Failures++;
	}
}

char const * Table_Text(int id)
{
	for (OpenTSLanguageString const & entry : OpenTSLanguageStrings) {
		if (entry.Id == id) {
			return(entry.Text);
		}
	}
	return(nullptr);
}

}	// namespace


int main(void)
{
	Check(OpenTSLanguageStringCount > 0, "the generated table carries at least one string");
	Check(OpenTSLanguageStringCount == (int)(sizeof(OpenTSLanguageStrings) / sizeof(OpenTSLanguageStrings[0])),
		"the generated count matches the table's actual length");

	// Spot-checked directly against language.rc's own STRINGTABLE text for these IDs.
	Check(Table_Text(TXT_OK) != nullptr && std::strcmp(Table_Text(TXT_OK), "OK") == 0,
		"TXT_OK reads back as \"OK\"");
	Check(Table_Text(TXT_CANCEL) != nullptr && std::strcmp(Table_Text(TXT_CANCEL), "Cancel") == 0,
		"TXT_CANCEL reads back as \"Cancel\"");
	Check(Table_Text(TXT_YES) != nullptr && std::strcmp(Table_Text(TXT_YES), "Yes") == 0,
		"TXT_YES reads back as \"Yes\"");
	Check(Table_Text(TXT_NO) != nullptr && std::strcmp(Table_Text(TXT_NO), "No") == 0,
		"TXT_NO reads back as \"No\"");

	// language.rc carries a placeholder space for TXT_NONE, same as every other id; the
	// sentinel behavior belongs to Fetch_String below, not the table itself.
	Check(Table_Text(TXT_NONE) != nullptr && std::strcmp(Table_Text(TXT_NONE), " ") == 0,
		"TXT_NONE's own table entry is the placeholder language.rc gives it");
	Check(Table_Text(-1) == nullptr, "an id no string uses has no entry");

	// Fetch_String is the game's own entry point onto this table; every id just spot-checked
	// against language.rc directly has to come back the same way through it.
	Check(std::strcmp(Fetch_String(TXT_OK), "OK") == 0, "Fetch_String(TXT_OK) is \"OK\"");
	Check(std::strcmp(Fetch_String(TXT_CANCEL), "Cancel") == 0, "Fetch_String(TXT_CANCEL) is \"Cancel\"");
	Check(std::strcmp(Fetch_String(TXT_YES), "Yes") == 0, "Fetch_String(TXT_YES) is \"Yes\"");
	Check(std::strcmp(Fetch_String(TXT_NO), "No") == 0, "Fetch_String(TXT_NO) is \"No\"");
	Check(std::strcmp(Fetch_String(TXT_NONE), "") == 0, "Fetch_String(TXT_NONE) is empty, the sentinel it is");
	Check(std::strcmp(Fetch_String(-1), "") == 0, "Fetch_String of an id with no string is empty");

	std::printf("\n%s\n", Failures == 0 ? "All checks passed." : "There were failures.");
	return(Failures == 0 ? 0 : 1);
}
