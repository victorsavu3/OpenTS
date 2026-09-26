/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

// Exercises the game directories without the engine or any game data: the folder list it
// is handed, the scan that covers every folder, and where a player's own files are read
// from and written to. Every file this uses is one the harness makes itself.

#include "always.h"
#include "win.h"

#include <cctype>
#include <cstdio>
#include <string>
#include <vector>

#include "cdfile.h"
#include "deploymentconfig.h"
#include "gamedirs.h"
#include "rawfile.h"

namespace {

int Failures = 0;

std::string Root;
char OriginalDirectory[MAX_PATH];

// Matches Terminate_Path's own choice of separator (gamedirs.cpp), so an expected value here
// is the same string the code under test builds.
#ifdef _WIN32
std::string const SEP = "\\";

std::string Lowercased(std::string const & name)
{
	return(name);
}
#else
std::string const SEP = "/";

// RawFileClass::Set_Name (rawfile.cpp) lowercases every name it is given on this platform, then
// updates it to the real on-disk name if a case-insensitive scan finds one; a name matching
// nothing on disk, including one this harness is about to create, keeps this lowercase form. An
// expected value naming such a file has to match that, not the case the harness wrote it with.
std::string Lowercased(std::string const & name)
{
	std::string result = name;
	for (char & ch : result) {
		ch = (char)std::tolower((unsigned char)ch);
	}
	return(result);
}
#endif


void Check(bool condition, char const * what)
{
	std::printf("%-62s %s\n", what, condition ? "ok" : "FAILED");

	if (!condition) {
		Failures++;
	}
}


void Check_List(std::vector<std::string> const & actual, std::vector<std::string> const & expected, char const * what)
{
	bool same = actual.size() == expected.size();

	for (unsigned int index = 0; same && index < actual.size(); index++) {
		same = actual[index] == expected[index];
	}

	Check(same, what);

	if (!same) {
		std::printf("    got:");
		for (std::string const & entry : actual) {
			std::printf(" [%s]", entry.c_str());
		}
		std::printf("\n    expected:");
		for (std::string const & entry : expected) {
			std::printf(" [%s]", entry.c_str());
		}
		std::printf("\n");
	}
}


// The file object keeps the name pointer it is handed rather than copying it, so the string
// it points into has to outlive it.
void Write_File(std::string const & path, char const * contents)
{
	RawFileClass file(path.c_str());

	file.Open(FileClass::WRITE);
	file.Write(contents, (int)strlen(contents));
	file.Close();
}


std::string Read_File(std::string const & path)
{
	RawFileClass file(path.c_str());

	if (!file.Is_Available()) {
		return(std::string());
	}

	int const size = file.Size();
	std::string contents(size, '\0');

	file.Open(FileClass::READ);
	file.Read(contents.data(), size);
	file.Close();

	return(contents);
}


bool File_Exists(std::string const & path)
{
	return(GetFileAttributes(path.c_str()) != INVALID_FILE_ATTRIBUTES);
}


void Make_Directory(std::string const & path)
{
	CreateDirectory(path.c_str(), NULL);
}


void Remove_Directory(std::string const & path)
{
	RemoveDirectory(path.c_str());
}


/*
 * Every case starts from the same empty tree, with no folders configured and the current
 * directory back at the root, so that one case cannot decide another's outcome.
 */
void Reset(void)
{
	CDFileClass::Clear_Search_Drives();
	Set_Data_Directory("");
	Set_User_Directory("");
	SetCurrentDirectory(Root.c_str());
}


void Test_Parsing(void)
{
	Check_List(Parse_Search_Folders("INI,MIX"), {"INI" + SEP, "MIX" + SEP}, "a plain list keeps its order");
	Check_List(Parse_Search_Folders("  INI  ,\tMIX "), {"INI" + SEP, "MIX" + SEP}, "surrounding whitespace is dropped");
	// A folder already written with an explicit separator is kept exactly as written, on
	// either platform, since Terminate_Path only adds one where none is there yet.
	Check_List(Parse_Search_Folders("INI\\,MIX/"), {"INI\\", "MIX/"}, "a separator already written is kept");

#ifdef _WIN32
	Check_List(Parse_Search_Folders("INI,ini\\,INI"), {"INI\\"}, "the same folder written differently is one folder");
#else
	// Only a folder Terminate_Path itself terminated compares equal to another one; "ini\"
	// keeps the backslash it was written with, which is not this platform's separator, so it
	// is its own entry rather than folding into "INI/".
	Check_List(Parse_Search_Folders("INI,ini\\,INI"), {"INI" + SEP, "ini\\"},
		"a folder written with the platform's own separator is deduplicated, one kept as written is not");
#endif

	Check_List(Parse_Search_Folders("INI,,MIX"), {"INI" + SEP, "MIX" + SEP}, "an empty entry is passed over");
	Check_List(Parse_Search_Folders(""), {}, "an empty list names no folders");
	Check_List(Parse_Search_Folders("   "), {}, "a list of whitespace names no folders");
	Check_List(Parse_Search_Folders(NULL), {}, "no list at all names no folders");
	Check_List(Parse_Search_Folders("D:"), {"D:"}, "a bare drive is left as it is");
	Check_List(Parse_Search_Folders("."), {}, "naming only the game's own directory adds no folder");
	Check_List(Parse_Search_Folders(".,Extra"), {"Extra" + SEP}, "the game's own directory is passed over in a longer list");
}


// The list the deployment's file supplies when it names no folders.
std::string Default_List(void)
{
	return(DeploymentConfigClass().SearchPaths);
}


void Test_Defaults(void)
{
	Reset();
	Init_Search_Folders(Default_List().c_str());

	Check(CDFileClass::Search_Path(0) != NULL && std::string(CDFileClass::Search_Path(0)) == "INI" + SEP,
		"with no configuration the INI folder is searched");
	Check(CDFileClass::Search_Path(1) != NULL && std::string(CDFileClass::Search_Path(1)) == "MIX" + SEP,
		"with no configuration the MIX folder is searched");
	Check(CDFileClass::Search_Path(2) != NULL && std::string(CDFileClass::Search_Path(2)) == "Maps" + SEP,
		"with no configuration the Maps folder is searched");
	Check(CDFileClass::Search_Path(3) == NULL, "nothing else is searched");
}


void Test_Configured_Folders(void)
{
	Reset();
	Init_Search_Folders("Data,More");

	Check(CDFileClass::Search_Path(0) != NULL && std::string(CDFileClass::Search_Path(0)) == "Data" + SEP,
		"a configured folder is searched");
	Check(CDFileClass::Search_Path(1) != NULL && std::string(CDFileClass::Search_Path(1)) == "More" + SEP,
		"configured folders keep the order they are written in");
	Check(CDFileClass::Search_Path(2) == NULL, "a configured list replaces the default folders");

	// Naming only the game's own directory is how a deployment asks for no other folder.
	Reset();
	Init_Search_Folders(".");

	Check(CDFileClass::Search_Path(0) == NULL, "naming only the game's own directory turns the default folders off");
}


void Test_Data_Directory(void)
{
	Reset();
	Set_Data_Directory((Root + SEP + "Data").c_str());

	Check(Apply_Game_Directories(), "a data directory that exists is accepted");
	Init_Search_Folders("Sorted");

	std::string const expected_data = Root + SEP + "Data" + SEP;
	std::string const expected_sorted = expected_data + "Sorted" + SEP;

	Check(CDFileClass::Search_Path(0) != NULL && std::string(CDFileClass::Search_Path(0)) == expected_data,
		"the data directory itself is searched");
	Check(CDFileClass::Search_Path(1) != NULL && std::string(CDFileClass::Search_Path(1)) == expected_sorted,
		"a folder it configures is searched inside it");

	Reset();
	Set_Data_Directory((Root + SEP + "Missing").c_str());
	Check(!Apply_Game_Directories(), "a data directory that is not there is refused");
}


void Test_User_Directory(void)
{
	Reset();
	Set_User_Directory((Root + SEP + "User" + SEP + "Fresh").c_str());

	Check(Apply_Game_Directories(), "a user directory is created when it is not there yet");
	Check(GetFileAttributes((Root + SEP + "User" + SEP + "Fresh").c_str()) != INVALID_FILE_ATTRIBUTES,
		"the created user directory is on the disk");

	std::string const expected_user = Root + SEP + "User" + SEP + "Fresh" + SEP;
	Check(CDFileClass::User_Path() != NULL && std::string(CDFileClass::User_Path()) == expected_user,
		"the file layer is told where the player's own files go");
	Check(CDFileClass::Search_Path(0) == NULL,
		"the user directory is not one of the searched folders");

	Check(User_File_Write_Name("SUN.INI") == expected_user + "SUN.INI",
		"a file the game writes goes to the user directory");

	Reset();
	Check(CDFileClass::User_Path() == NULL,
		"with no user directory the file layer has none either");
	Check(User_File_Write_Name("SUN.INI") == "SUN.INI",
		"with no user directory a written file keeps its plain name");
}


void Test_Search_Files(void)
{
	Reset();

	Write_File(Root + SEP + "ALPHA.MPR", "");
	Write_File(Root + SEP + "INI" + SEP + "BRAVO.MPR", "");
	Write_File(Root + SEP + "INI" + SEP + "ALPHA.MPR", "");
	Write_File(Root + SEP + "MIX" + SEP + "CHARLIE.MPR", "");

	Init_Search_Folders(Default_List().c_str());

	Check_List(Search_Files("*.MPR"), {Lowercased("ALPHA.MPR"), Lowercased("BRAVO.MPR"), Lowercased("CHARLIE.MPR")},
		"a scan covers every folder, reports a name once, and sorts it");

	/*
	 * The scan and an ordinary open have to agree, or the game would list one file and load
	 * another. Both walk the folders in the same order, so the game's own copy wins.
	 */
	CDFileClass found("ALPHA.MPR");
	Check(std::string(found.File_Name()) == Lowercased("ALPHA.MPR"),
		"opening a name the scan reported lands on the copy the scan saw");

	CDFileClass sorted("CHARLIE.MPR");
	Check(std::string(sorted.File_Name()) == "MIX" + SEP + Lowercased("CHARLIE.MPR"),
		"a name held only by a searched folder opens from that folder");
}


void Test_Writes_Do_Not_Search(void)
{
	Reset();
	Write_File(Root + SEP + "MIX" + SEP + "WRITTEN.DAT", "shipped");
	Init_Search_Folders(Default_List().c_str());

	/*
	 * A file opened for writing must never be looked for anywhere but the current directory:
	 * a deployment's folders are read from, not written to.
	 */
	CDFileClass file;
	file.Open("WRITTEN.DAT", FileClass::READ|FileClass::WRITE);
	std::string const written = file.File_Name();
	file.Close();

	Check(written == Lowercased("WRITTEN.DAT"), "a read-write open does not settle on a searched folder");

	Check(GetFileAttributes((Root + SEP + Lowercased("WRITTEN.DAT")).c_str()) != INVALID_FILE_ATTRIBUTES,
		"the written file is in the current directory");

	WIN32_FILE_ATTRIBUTE_DATA shipped;
	GetFileAttributesEx((Root + SEP + "MIX" + SEP + Lowercased("WRITTEN.DAT")).c_str(), GetFileExInfoStandard, &shipped);
	Check(shipped.nFileSizeLow == 7, "the copy in the searched folder is untouched");
}


/*
 * A file named with a directory of its own, looked up while folders are searched, makes the
 * search build a pathname out of two full paths. That pair does not have to fit in one, and
 * once it does not the search has to pass it over rather than build it anyway.
 */
void Test_Long_Names(void)
{
	Reset();

	std::string long_folder = Root + SEP;
	while (long_folder.length() < 150) {
		long_folder += "x";
	}
	long_folder += SEP;

	CDFileClass::Add_Search_Drive(long_folder.c_str());

	std::string const absolute = Root + SEP + "ALPHA.MPR";
	Write_File(absolute, "");

	CDFileClass file(absolute.c_str());
	Check(std::string(file.File_Name()) == Root + SEP + Lowercased("ALPHA.MPR"),
		"a file named with its own directory is found while long folders are searched");

	// "As it was given" only holds where nothing lowercases the name first; see Lowercased.
	CDFileClass missing((Root + SEP + "NOTHERE.MPR").c_str());
	Check(std::string(missing.File_Name()) == Root + SEP + Lowercased("NOTHERE.MPR"),
		"a name that no folder holds comes back as it was given");

	Check_List(Search_Files("*.MPR"), {Lowercased("ALPHA.MPR")}, "a scan passes over a folder it cannot build a name in");
}


/*
 * The file layer places what the game writes and finds what it reads. These are the rules a
 * caller never states, so they are checked here rather than at any one of them.
 */
void Test_The_File_Layer_Places_Written_Files(void)
{
	Reset();
	Set_User_Directory((Root + SEP + "User" + SEP + "Own").c_str());
	Apply_Game_Directories();
	Init_Search_Folders(Default_List().c_str());

	std::string const own = Root + SEP + "User" + SEP + "Own" + SEP;

	CDFileClass written("OWN.DAT");
	written.Open(FileClass::WRITE);
	written.Write("mine", 4);
	written.Close();

	Check(std::string(written.File_Name()) == own + Lowercased("OWN.DAT"), "a written file is named in the user directory");
	Check(File_Exists(own + Lowercased("OWN.DAT")), "a written file is in the user directory");
	Check(!File_Exists(Root + SEP + "OWN.DAT"), "a written file is not beside the game");

	/*
	 * A second object, made once both copies exist, so that what answers is the search and
	 * not the object that did the writing.
	 */
	Write_File(Root + SEP + "MIX" + SEP + "SHARED.DAT", "shipped");
	Write_File(own + "SHARED.DAT", "own");

	CDFileClass shared("SHARED.DAT");
	Check(Read_File(shared.File_Name()) == "own", "a read prefers the player's own copy");

	// A created file is read back from where it was created, which is what a game storing
	// its progress does every time it starts.
	CDFileClass progress("PROGRESS.INI");
	Check(!progress.Is_Available(), "a file the player has never had is not there yet");
	progress.Create();
	progress.Close();

	CDFileClass reopened("PROGRESS.INI");
	Check(reopened.Is_Available(), "a created file is found again");
	Check(std::string(reopened.File_Name()) == own + Lowercased("PROGRESS.INI"), "a created file is found in the user directory");
}


void Test_The_File_Layer_Deletes_Only_The_Player_Copy(void)
{
	Reset();
	Set_User_Directory((Root + SEP + "User" + SEP + "Own").c_str());
	Apply_Game_Directories();
	Init_Search_Folders(Default_List().c_str());

	std::string const own = Root + SEP + "User" + SEP + "Own" + SEP;

	Write_File(Root + SEP + "MIX" + SEP + "GONE.DAT", "shipped");
	Write_File(own + "GONE.DAT", "own");

	CDFileClass discard("GONE.DAT");
	discard.Delete();

	Check(!File_Exists(own + Lowercased("GONE.DAT")), "the player's own copy is thrown away");
	Check(File_Exists(Root + SEP + "MIX" + SEP + Lowercased("GONE.DAT")), "the copy a deployment ships is left alone");

	CDFileClass again("GONE.DAT");
	Check(Read_File(again.File_Name()) == "shipped", "what a deployment ships answers once the player's copy is gone");
}


/*
 * A deployment ships default hotkeys in a folder it searches, and the player saves their own
 * over the top. Throwing the player's away has to leave the deployment's alone, or the reset
 * takes with it the very defaults it is meant to fall back on.
 */
void Test_Resetting_Keeps_The_Shipped_Default(void)
{
	Reset();
	Set_User_Directory((Root + SEP + "User" + SEP + "Own").c_str());
	Apply_Game_Directories();
	Init_Search_Folders(Default_List().c_str());

	Write_File(Root + SEP + "INI" + SEP + "KEYBOARD.INI", "shipped");

	// A player who has never saved their own asks for the defaults back.
	CDFileClass untouched("KEYBOARD.INI");
	Check(std::string(untouched.File_Name()) == "INI" + SEP + Lowercased("KEYBOARD.INI"),
		"a player with none of their own reads the shipped default");
	untouched.Delete();
	Check(File_Exists(Root + SEP + "INI" + SEP + Lowercased("KEYBOARD.INI")),
		"a reset with nothing of the player's own leaves the shipped default");

	// And now one who has.
	Write_File(Root + SEP + "User" + SEP + "Own" + SEP + "KEYBOARD.INI", "mine");

	CDFileClass owned("KEYBOARD.INI");
	Check(Read_File(owned.File_Name()) == "mine", "the player's own hotkeys are the ones read");
	owned.Delete();

	Check(!File_Exists(Root + SEP + "User" + SEP + "Own" + SEP + Lowercased("KEYBOARD.INI")), "a reset throws the player's own away");
	Check(File_Exists(Root + SEP + "INI" + SEP + Lowercased("KEYBOARD.INI")), "a reset leaves the shipped default");

	CDFileClass fallback("KEYBOARD.INI");
	Check(Read_File(fallback.File_Name()) == "shipped", "the shipped default answers again after a reset");

	DeleteFile((Root + SEP + "INI" + SEP + Lowercased("KEYBOARD.INI")).c_str());
}


void Test_A_Name_With_A_Directory_Is_Left_Alone(void)
{
	Reset();
	Set_User_Directory((Root + SEP + "User" + SEP + "Own").c_str());
	Apply_Game_Directories();

	CDFileClass rooted(("MIX" + SEP + "ROOTED.DAT").c_str());
	rooted.Open(FileClass::WRITE);
	rooted.Write("here", 4);
	rooted.Close();

	Check(std::string(rooted.File_Name()) == "MIX" + SEP + Lowercased("ROOTED.DAT"), "a name with a directory keeps it");
	Check(File_Exists(Root + SEP + "MIX" + SEP + Lowercased("ROOTED.DAT")), "a name with a directory is written where it says");
	Check(!File_Exists(Root + SEP + "User" + SEP + "Own" + SEP + Lowercased("ROOTED.DAT")), "a name with a directory is not moved");
}


void Test_Placing_A_File_Is_Repeatable(void)
{
	Reset();
	Set_User_Directory((Root + SEP + "User" + SEP + "Own").c_str());
	Apply_Game_Directories();

	CDFileClass file("AGAIN.DAT");
	file.Open(FileClass::WRITE);
	file.Close();
	std::string const once = file.File_Name();

	file.Open(FileClass::WRITE);
	file.Close();

	Check(std::string(file.File_Name()) == once, "opening a file for writing twice names it the same place");

	// The buffered path opens the file a second time itself, with read access added.
	CDFileClass buffered("BUFFERED.DAT");
	buffered.Cache(1024);
	buffered.Open(FileClass::WRITE);
	buffered.Write("cached", 6);
	buffered.Close();

	Check(File_Exists(Root + SEP + "User" + SEP + "Own" + SEP + Lowercased("BUFFERED.DAT")), "a buffered write lands in the user directory");
}


void Test_Without_A_User_Directory_Nothing_Moves(void)
{
	Reset();
	Init_Search_Folders(Default_List().c_str());

	Write_File(Root + SEP + "MIX" + SEP + "STILL.DAT", "shipped");

	CDFileClass written("STILL.DAT");
	written.Open(FileClass::WRITE);
	written.Write("here", 4);
	written.Close();

	Check(std::string(written.File_Name()) == Lowercased("STILL.DAT"), "a written file keeps its plain name");
	Check(File_Exists(Root + SEP + Lowercased("STILL.DAT")), "a written file lands beside the game");
	Check(Read_File(Root + SEP + "MIX" + SEP + "STILL.DAT") == "shipped", "a searched folder's copy is untouched");

	CDFileClass discard("STILL.DAT");
	discard.Delete();

	Check(!File_Exists(Root + SEP + Lowercased("STILL.DAT")), "a delete takes the copy beside the game");
	Check(File_Exists(Root + SEP + "MIX" + SEP + Lowercased("STILL.DAT")), "a delete leaves the searched folder's copy");

	CDFileClass shipped("STILL.DAT");
	Check(std::string(shipped.File_Name()) == "MIX" + SEP + Lowercased("STILL.DAT"), "a read still falls through to the searched folders");
}


/*
 * Saved games are the one thing the game both writes and browses, so they keep to a folder of
 * their own that is named outright rather than searched for.
 */
void Test_Saved_Games_Folder(void)
{
	Reset();
	Init_Search_Folders(Default_List().c_str());

	Check(Saved_Game_Name("SAVE0001.SAV") == "Saved Games" + SEP + "SAVE0001.SAV",
		"a saved game is named inside the folder saved games are kept in");
	Check(File_Exists(Root + SEP + "Saved Games"),
		"asking for a saved game makes the folder to keep it in");

	for (int index = 0; ; index++) {
		char const * path = CDFileClass::Search_Path(index);
		if (path == NULL) {
			break;
		}

		Check(std::string(path).find("Saved Games") == std::string::npos,
			"the folder saved games are kept in is not one of the searched folders");
	}

	Reset();
	Set_User_Directory((Root + SEP + "User" + SEP + "Saves").c_str());
	Apply_Game_Directories();

	std::string const expected = Root + SEP + "User" + SEP + "Saves" + SEP + "Saved Games";
	Check(Saved_Game_Name("SAVE0002.SAV") == expected + SEP + "SAVE0002.SAV",
		"a user directory takes the saved games with it");
	Check(File_Exists(expected),
		"the folder is made inside the user directory");

	/*
	 * A pattern is named the same way a file is, since the listing scans the one folder rather
	 * than every folder the game reads from.
	 */
	Check(Saved_Game_Name("*.SAV") == expected + SEP + "*.SAV",
		"a pattern is named in the same folder the saved games are");
}


/*
 * Screen captures keep to a folder of their own on the same terms, except that the folder is
 * made on every request rather than once, so removing it mid-session costs nothing.
 */
void Test_Screenshots_Folder(void)
{
	Reset();
	Init_Search_Folders(Default_List().c_str());

	Check(Screenshot_Name("SCRN0000.pcx") == "Screenshots" + SEP + "SCRN0000.pcx",
		"a capture is named inside the folder screen captures are kept in");
	Check(File_Exists(Root + SEP + "Screenshots"),
		"asking for a capture makes the folder to keep it in");

	for (int index = 0; ; index++) {
		char const * path = CDFileClass::Search_Path(index);
		if (path == NULL) {
			break;
		}

		Check(std::string(path).find("Screenshots") == std::string::npos,
			"the folder screen captures are kept in is not one of the searched folders");
	}

	Reset();
	Set_User_Directory((Root + SEP + "User" + SEP + "Shots").c_str());
	Apply_Game_Directories();

	std::string const expected = Root + SEP + "User" + SEP + "Shots" + SEP + "Screenshots";
	Check(Screenshot_Name("SCRN0001.png") == expected + SEP + "SCRN0001.png",
		"a user directory takes the captures with it");
	Check(File_Exists(expected),
		"the folder is made inside the user directory");

	Remove_Directory(expected);
	Check(!File_Exists(expected), "the folder can be taken away while the game runs");
	Check(Screenshot_Name("SCRN0002.png") == expected + SEP + "SCRN0002.png",
		"and naming the next capture still answers");
	Check(File_Exists(expected), "which makes the folder again");
}


bool Make_Root(void)
{
	char temp[MAX_PATH];
	if (GetTempPath(sizeof(temp), temp) == 0) {
		return(false);
	}

	char name[MAX_PATH];
	std::snprintf(name, sizeof(name), "%sopents-gamedirs-%u", temp, (unsigned)GetCurrentProcessId());
	Root = name;

	Make_Directory(Root);
	Make_Directory(Root + SEP + "INI");
	Make_Directory(Root + SEP + "MIX");
	Make_Directory(Root + SEP + "Data");
	Make_Directory(Root + SEP + "User");

	return(SetCurrentDirectory(Root.c_str()) != 0);
}


void Remove_Root(void)
{
	SetCurrentDirectory(OriginalDirectory);

	// The tree is shallow and entirely this harness's own, so it is removed by name.
	char command[MAX_PATH + 32];
#ifdef _WIN32
	std::snprintf(command, sizeof(command), "cmd /c rd /s /q \"%s\"", Root.c_str());
#else
	std::snprintf(command, sizeof(command), "rm -rf \"%s\"", Root.c_str());
#endif
	system(command);
}

}


int main(void)
{
	GetCurrentDirectory(sizeof(OriginalDirectory), OriginalDirectory);

	if (!Make_Root()) {
		std::printf("could not create the working directory\n");
		return(1);
	}

	std::printf("Working in %s\n\n", Root.c_str());

	Test_Parsing();
	Test_Defaults();
	Test_Configured_Folders();
	Test_Data_Directory();
	Test_User_Directory();
	Test_Search_Files();
	Test_Writes_Do_Not_Search();
	Test_Long_Names();
	Test_The_File_Layer_Places_Written_Files();
	Test_The_File_Layer_Deletes_Only_The_Player_Copy();
	Test_Resetting_Keeps_The_Shipped_Default();
	Test_A_Name_With_A_Directory_Is_Left_Alone();
	Test_Placing_A_File_Is_Repeatable();
	Test_Without_A_User_Directory_Nothing_Moves();
	Test_Saved_Games_Folder();
	Test_Screenshots_Folder();

	Reset();
	Remove_Root();

	std::printf("\n%s\n", Failures == 0 ? "All checks passed." : "There were failures.");
	return(Failures == 0 ? 0 : 1);
}
