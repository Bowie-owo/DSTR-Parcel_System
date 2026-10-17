// =====================================================================
//  common/Utils.cpp
//  Implementation of the helpers shared by all task modules.
// =====================================================================
#include "Utils.hpp"

#include <cctype>
#include <ctime>
#include <iostream>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

using namespace std;

namespace common
{
// ---------------- console input ----------------
string readLine(const string &prompt)
{
    cout << prompt;
    string line;
    if (!getline(cin, line))
        return "";
    return trim(line);
}

// Rejects blanks, letters, symbols, decimals and out-of-range numbers.
int readInt(const string &prompt, int minValue, int maxValue)
{
    while (true)
    {
        string line = readLine(prompt);
        if (cin.eof())
            return minValue;
        if (line.empty() || line.size() > 9 ||
            line.find_first_not_of("0123456789") != string::npos)
        {
            cout << "  Invalid input: please enter a number.\n";
            continue;
        }
        int value = stoi(line);
        if (value < minValue || value > maxValue)
        {
            cout << "  Out of range: choose between " << minValue << " and " << maxValue << ".\n";
            continue;
        }
        return value;
    }
}

bool readYesNo(const string &prompt)
{
    while (true)
    {
        string line = readLine(prompt + " (Y/N): ");
        if (cin.eof())
            return false;
        if (line == "Y" || line == "y")
            return true;
        if (line == "N" || line == "n")
            return false;
        cout << "  Please enter Y or N.\n";
    }
}

string pickFromList(const string &title, const string options[], int optionCount)
{
    cout << "\n " << title << "\n";
    for (int i = 0; i < optionCount; i++)
        cout << "   " << (i + 1) << ". " << options[i] << "\n";
    int choice = readInt(" Choice: ", 1, optionCount);
    if (cin.eof())
        return "";
    return options[choice - 1];
}

// ---------------- text helpers ----------------
string trim(const string &text)
{
    size_t firstChar = text.find_first_not_of(" \t\r\n");
    if (firstChar == string::npos)
        return "";
    size_t lastChar = text.find_last_not_of(" \t\r\n");
    return text.substr(firstChar, lastChar - firstChar + 1);
}

string toUpper(string text)
{
    for (size_t i = 0; i < text.size(); i++)
        text[i] = (char)toupper((unsigned char)text[i]);
    return text;
}

bool isValidId(const string &text, size_t maxLength)
{
    if (text.empty() || text.size() > maxLength)
        return false;
    for (size_t i = 0; i < text.size(); i++)
    {
        unsigned char c = (unsigned char)text[i];
        if (!isalnum(c) && c != '-' && c != '_')
            return false;
    }
    return true;
}

// ---------------- date / time ----------------
string currentDateTime()
{
    time_t now = time(nullptr);
    tm *localTime = localtime(&now);
    if (localTime == nullptr)
        return "";
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", localTime);
    return buffer;
}

// ---------------- file helper ----------------
void makeParentDir(const string &path)
{
    size_t separatorPos = path.find_last_of("/\\");
    if (separatorPos == string::npos || separatorPos == 0)
        return;
    string folder = path.substr(0, separatorPos);
#ifdef _WIN32
    _mkdir(folder.c_str());
#else
    mkdir(folder.c_str(), 0755);
#endif
}
} // namespace common