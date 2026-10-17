// =====================================================================
//  common/Utils.hpp
//  Helpers shared by ALL task modules: validated console input, text
//  handling, date/time, folder creation. Everything lives in namespace
//  "common" so it cannot clash with the helpers still inside main.cpp.
//
//  Usage in a module's .cpp file:
//      #include "../common/Utils.hpp"
//      using namespace common;
// =====================================================================
#pragma once

#include <functional>
#include <string>

namespace common
{
// ---------------- console input (all re-prompt on bad input) ----------------
std::string readLine(const std::string &prompt);                    // trimmed line, "" on EOF
int readInt(const std::string &prompt, int minValue, int maxValue); // whole number in range
bool readYesNo(const std::string &prompt);                          // Y/N question

// Shows a numbered list; returns the chosen entry ("" if input closed).
std::string pickFromList(const std::string &title, const std::string options[], int optionCount);

// ---------------- text helpers ----------------
std::string trim(const std::string &text);  // remove leading/trailing whitespace
std::string toUpper(std::string text);      // "cc01" -> "CC01"

// IDs: 1..maxLength characters, letters / digits / '-' / '_' only.
bool isValidId(const std::string &text, size_t maxLength = 15);

// ---------------- date / time ----------------
std::string currentDateTime(); // "2026-10-16 09:15" (local time)

// ---------------- file helper ----------------
// Creates the folder part of a path if missing (one level, e.g. "data").
void makeParentDir(const std::string &path);

// ---------------- activity hook ----------------
// Any module can report a change; Task 4 (Activity History & Undo) registers a
// function of this type to record it, e.g. ("ASSIGN", "CC01|PCL001").
using ActivityLogger = std::function<void(const std::string &action, const std::string &details)>;
} // namespace common