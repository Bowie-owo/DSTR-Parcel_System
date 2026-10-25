// =====================================================================
//  Task 4 - Parcel Activity and Record Management Module
//  task4_activity_history/activityManager.cpp
//
//  Sections:
//    1. Local helpers
//    2. Construction and file handling
//    3. Recording activities                 (Requirement 1)
//    4. Display and review                   (Requirement 2, 3)
//    5. Undo / reverse                       (Requirement 4, 5)
//    6. Shared instance + Task 3 hook
//    7. Menu                                 (activityMenu)
// =====================================================================
#include "activityManager.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "../common/Utils.hpp"
#include "../task3_resources/resourceManager.hpp"

using namespace std;
using namespace common;

// =====================================================================
//  1. Local helpers
// =====================================================================
namespace
{
const string NONE = "-";       // placeholder for "not applicable"
const char FILE_SEPARATOR = '\t'; // data-file separator (details contain '|')

// Current local date and time as "YYYY-MM-DD HH:MM", from the shared helper in
// common/Utils (the activity number keeps same-minute activities in order).
// Falls back to "-" if the clock cannot be read.
string currentTimestamp()
{
    string now = currentDateTime();
    return now.empty() ? NONE : now;
}

// Splits 'text' at every 'separator'. Fills parts[0..maxParts-1] and returns the
// TOTAL number of pieces found (it can be larger than maxParts).
int splitFields(const string &text, char separator, string parts[], int maxParts)
{
    int pieceCount = 0;
    string current;
    for (size_t i = 0; i <= text.size(); i++)
    {
        if (i == text.size() || text[i] == separator)
        {
            if (pieceCount < maxParts)
                parts[pieceCount] = current;
            pieceCount++;
            current.clear();
        }
        else
            current += text[i];
    }
    return pieceCount;
}

// True when 'text' is one or more digits only.
bool isNumber(const string &text)
{
    return !text.empty() && text.find_first_not_of("0123456789") == string::npos;
}

// Makes a value safe for the data file: no tabs/newlines, never empty.
void sanitize(string &text)
{
    for (size_t i = 0; i < text.size(); i++)
        if (text[i] == '\t' || text[i] == '\n' || text[i] == '\r')
            text[i] = ' ';
    text = trim(text);
    if (text.empty())
        text = NONE;
}

// Parses one data-file line into a record. Returns false for a malformed line.
// Line layout (tab separated):
//   sequence  time  action  item  resource  previous  new  undone(0/1)  undoneTime  details
bool parseRecordLine(const string &line, ActivityRecord &record)
{
    string f[10];
    if (splitFields(line, FILE_SEPARATOR, f, 10) != 10)
        return false;
    if (!isNumber(f[0]) || f[0].size() > 8 || f[2].empty() || (f[7] != "0" && f[7] != "1"))
        return false;

    record.sequence = stoi(f[0]);
    record.timestamp = f[1];
    record.action = f[2];
    record.itemId = f[3];
    record.resourceId = f[4];
    record.previousStatus = f[5];
    record.newStatus = f[6];
    record.undone = (f[7] == "1");
    record.undoneTime = f[8];
    record.details = f[9];
    return true;
}

// ---- printing --------------------------------------------------------
void printActivityTableHeader()
{
    cout << left << setw(6) << "No." << setw(21) << "Time" << setw(18) << "Action"
         << setw(10) << "Item" << setw(10) << "Resource" << setw(28) << "Status change"
         << "State\n";
    cout << string(100, '-') << "\n";
}

void printTableRow(const ActivityRecord &r)
{
    string change = r.previousStatus + " -> " + r.newStatus;
    cout << left << setw(6) << r.sequence << setw(21) << r.timestamp << setw(18) << r.action
         << setw(10) << r.itemId << setw(10) << r.resourceId << setw(28) << change
         << (r.undone ? "Reversed" : "Active") << "\n";
}

// Every field of one activity (used by "view most recent" and before an undo).
void printDetail(const ActivityRecord &r)
{
    cout << "\n  Activity No.    : " << r.sequence << "\n"
         << "  Time            : " << r.timestamp << "\n"
         << "  Action          : " << r.action << "\n"
         << "  Parcel/Request  : " << r.itemId << "\n"
         << "  Resource        : " << r.resourceId << "\n"
         << "  Previous status : " << r.previousStatus << "\n"
         << "  New status      : " << r.newStatus << "\n"
         << "  Details         : " << r.details << "\n"
         << "  State           : "
         << (r.undone ? "Reversed at " + r.undoneTime : string("Active")) << "\n";
}

// True when the ID appears as the parcel/request or the resource of the activity.
bool recordMatchesId(const ActivityRecord &r, const string &id)
{
    return r.itemId == id || r.resourceId == id;
}
} // namespace

// =====================================================================
//  2. Construction and file handling
// =====================================================================
ActivityManager::ActivityManager(const string &path)
    : nextSequence(1), undoing(false), filePath(path)
{
    load();
}

// Loads the history file. Malformed lines are skipped with a message; a missing
// file is not an error (the history simply starts empty).
bool ActivityManager::load()
{
    history.clear();
    nextSequence = 1;

    ifstream inputFile(filePath);
    if (!inputFile)
        return false;

    // The file is written newest-first. Pushing every line onto a temporary
    // stack and popping them back REVERSES that order, so the main stack is
    // rebuilt with the newest activity on top again.
    ActivityStack reversedOrder;
    string line;
    int lineNumber = 0, skippedLines = 0;
    while (getline(inputFile, line))
    {
        lineNumber++;
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (line.empty())
            continue;

        ActivityRecord record;
        if (!parseRecordLine(line, record))
        {
            cout << "  [activity] Skipped invalid line " << lineNumber << " in " << filePath << "\n";
            skippedLines++;
            continue;
        }
        reversedOrder.push(record);
    }

    ActivityRecord record;
    while (reversedOrder.pop(record))
        history.push(record);

    if (!history.isEmpty())
        nextSequence = history.top()->data.sequence + 1;
    return skippedLines == 0;
}

// Writes the whole history, newest first (creates the folder on first use).
bool ActivityManager::save() const
{
    ofstream outputFile(filePath);
    if (!outputFile)
    {
        makeParentDir(filePath);
        outputFile.open(filePath);
        if (!outputFile)
            return false;
    }
    for (ActivityNode *node = history.top(); node != nullptr; node = node->next)
    {
        const ActivityRecord &r = node->data;
        outputFile << r.sequence << FILE_SEPARATOR << r.timestamp << FILE_SEPARATOR << r.action
                   << FILE_SEPARATOR << r.itemId << FILE_SEPARATOR << r.resourceId
                   << FILE_SEPARATOR << r.previousStatus << FILE_SEPARATOR << r.newStatus
                   << FILE_SEPARATOR << (r.undone ? 1 : 0) << FILE_SEPARATOR << r.undoneTime
                   << FILE_SEPARATOR << r.details << '\n';
    }
    return true;
}

// =====================================================================
//  3. Recording activities
// =====================================================================

// Stamps the record (number + time), pushes it on the stack and saves.
// Nothing is recorded while an undo is running: the undo calls Task 3
// (remove / updateStatus), which fires the hook again, and that echo must not
// be stored as a brand-new activity.
void ActivityManager::addRecord(ActivityRecord &record)
{
    if (undoing)
        return;

    sanitize(record.action);
    sanitize(record.itemId);
    sanitize(record.resourceId);
    sanitize(record.previousStatus);
    sanitize(record.newStatus);
    sanitize(record.details);
    record.action = toUpper(record.action);
    record.itemId = toUpper(record.itemId);
    record.resourceId = toUpper(record.resourceId);

    record.sequence = nextSequence;
    record.timestamp = currentTimestamp();
    record.undone = false;
    record.undoneTime = NONE;

    if (!history.push(record))
    {
        cout << "  [activity] Warning: out of memory, activity not recorded.\n";
        return;
    }
    nextSequence++;
    save();
}

// Task 3 hook. Turns the (action, details) text into a full record.
void ActivityManager::record(const string &action, const string &details)
{
    ActivityRecord rec;
    rec.action = action;
    rec.details = details;

    string parts[4];
    int pieceCount = splitFields(details, '|', parts, 4);

    if (action == "REGISTER" && pieceCount >= 1) // "id|type"
    {
        rec.resourceId = parts[0];
        rec.newStatus = "Registered";
    }
    else if (action == "STATUS" && pieceCount >= 3) // "id|oldStatus|newStatus"
    {
        rec.resourceId = parts[0];
        rec.previousStatus = parts[1];
        rec.newStatus = parts[2];
    }
    else if ((action == "ASSIGN" || action == "RELEASE") && pieceCount >= 2) // "id|item"
    {
        rec.resourceId = parts[0];
        rec.itemId = parts[1];
        rec.previousStatus = (action == "ASSIGN") ? "Available" : "Occupied";
        rec.newStatus = (action == "ASSIGN") ? "Occupied" : "Available";
    }
    else if (action == "INTERRUPT" && pieceCount >= 3) // "id|item|newStatus"
    {
        rec.resourceId = parts[0];
        rec.itemId = parts[1];
        rec.previousStatus = "Occupied";
        rec.newStatus = parts[2];
    }
    else if (action == "REMOVE" && pieceCount >= 1) // "id"
    {
        rec.resourceId = parts[0];
        rec.newStatus = "Removed";
    }
    // Any other action is stored as it is, with the raw details text.
    addRecord(rec);
}

// Full version used by Task 1 and Task 2.
void ActivityManager::recordActivity(const string &action, const string &itemId,
                                     const string &resourceId, const string &previousStatus,
                                     const string &newStatus, const string &details)
{
    ActivityRecord rec;
    rec.action = action;
    rec.itemId = itemId;
    rec.resourceId = resourceId;
    rec.previousStatus = previousStatus;
    rec.newStatus = newStatus;
    rec.details = details;
    addRecord(rec);
}

// =====================================================================
//  4. Display and review
// =====================================================================

// Shows up to maxItems activities from the top of the stack (newest first).
void ActivityManager::displayRecent(int maxItems) const
{
    if (history.isEmpty())
    {
        cout << "\n  No activity history recorded yet.\n";
        return;
    }
    cout << "\n=========== RECENT ACTIVITIES (newest first) ===========\n";
    printActivityTableHeader();
    int shown = 0;
    for (ActivityNode *node = history.top(); node != nullptr && shown < maxItems; node = node->next)
    {
        printTableRow(node->data);
        shown++;
    }
    cout << "\nShowing " << shown << " of " << history.size() << " activity record(s).\n";
}

// History of one Parcel / Request / Resource ID in the order it happened.
void ActivityManager::displayHistoryFor(const string &idInput) const
{
    string id = toUpper(trim(idInput));
    if (!isValidId(id))
    {
        cout << "\n  Invalid ID (1-15 letters, digits, '-' or '_').\n";
        return;
    }
    if (history.isEmpty())
    {
        cout << "\n  No activity history recorded yet.\n";
        return;
    }

    // Walking the stack gives newest -> oldest. Pushing the matches onto a
    // second stack and popping them reverses that to oldest -> newest.
    ActivityStack matches;
    int found = 0;
    for (ActivityNode *node = history.top(); node != nullptr; node = node->next)
        if (recordMatchesId(node->data, id))
        {
            matches.push(node->data);
            found++;
        }

    if (found == 0)
    {
        cout << "\n  No activity found for " << id << ".\n";
        return;
    }

    cout << "\n=========== HISTORY FOR " << id << " (oldest first) ===========\n";
    printActivityTableHeader();
    ActivityRecord record, newest;
    while (matches.pop(record))
    {
        printTableRow(record);
        newest = record; // the last one popped is the most recent
    }
    cout << "\n" << found << " activity record(s). Most recent: No. " << newest.sequence << " "
         << newest.action << " (" << newest.previousStatus << " -> " << newest.newStatus << ")\n";
}

// The newest activity on the stack, in full detail.
void ActivityManager::displayMostRecent() const
{
    ActivityRecord record;
    if (!history.peek(record))
    {
        cout << "\n  No activity history recorded yet.\n";
        return;
    }
    cout << "\n=========== MOST RECENT ACTIVITY ===========";
    printDetail(record);
}

// Shows the activity an undo would act on. Returns false when there is none.
bool ActivityManager::displayUndoCandidate() const
{
    ActivityNode *node = findLatestActive();
    if (node == nullptr)
        return false;
    cout << "\n=========== ACTIVITY TO UNDO ===========";
    printDetail(node->data);
    if (!isReversible(node->data.action))
        cout << "\n  Note: this type of activity cannot be reversed.\n";
    return true;
}

// Shows one activity by its number. Returns false for an unknown number.
bool ActivityManager::displayActivity(int sequence) const
{
    ActivityNode *node = findBySequence(sequence);
    if (node == nullptr)
        return false;
    cout << "\n=========== ACTIVITY No. " << sequence << " ===========";
    printDetail(node->data);
    return true;
}

// =====================================================================
//  5. Undo / reverse
// =====================================================================

// Only these activities have a matching reverse operation in Task 3 today:
//   REGISTER -> remove the resource again
//   STATUS   -> put the previous status back
// ASSIGN, RELEASE, INTERRUPT and REMOVE need extra Task 3 functions first.
bool ActivityManager::isReversible(const string &action)
{
    return action == "REGISTER" || action == "STATUS";
}

// Newest activity that has not been reversed yet (nullptr if none).
ActivityNode *ActivityManager::findLatestActive() const
{
    for (ActivityNode *node = history.top(); node != nullptr; node = node->next)
        if (!node->data.undone)
            return node;
    return nullptr;
}

// Linear search by activity number (nullptr if it does not exist).
ActivityNode *ActivityManager::findBySequence(int sequence) const
{
    for (ActivityNode *node = history.top(); node != nullptr; node = node->next)
        if (node->data.sequence == sequence)
            return node;
    return nullptr;
}

// Performs the real reverse operation through Task 3. 'undoing' is raised
// first so the hook calls fired by Task 3 are ignored by record()/addRecord().
bool ActivityManager::applyReversal(const ActivityRecord &record, string &err)
{
    ResourceManager &resources = getResourceManager();
    Resource *target = resources.find(record.resourceId);
    if (target == nullptr)
    {
        err = "Resource " + record.resourceId + " no longer exists, so there is nothing to reverse.";
        return false;
    }

    undoing = true;
    bool success = false;
    if (record.action == "REGISTER")
    {
        // Undo a registration = remove the resource (fails if it is occupied).
        success = resources.remove(record.resourceId, err);
    }
    else // STATUS
    {
        // The resource must still be in the status this activity set. If something
        // else changed it, reversing would leave an invalid or inconsistent status.
        if (target->status != record.newStatus)
            err = "Resource " + record.resourceId + " is now '" + target->status +
                  "' (expected '" + record.newStatus + "'). Reversing would give an invalid status.";
        else
            success = resources.updateStatus(record.resourceId, record.previousStatus, err);
    }
    undoing = false; // always lowered again, success or not
    return success;
}

// Checks every rule, then reverses the activity and marks it as reversed.
bool ActivityManager::reverseActivity(ActivityNode *node, string &message, string &err)
{
    ActivityRecord &record = node->data;
    ActivityNode *latest = findLatestActive();
    string number = "No. " + to_string(record.sequence);

    if (record.undone)
        err = "Activity " + number + " has already been reversed (" + record.undoneTime + ").";
    else if (!isReversible(record.action))
        err = "Activity " + number + " (" + record.action + ") cannot be reversed. "
              "Only REGISTER and STATUS activities can be undone at the moment.";
    else if (node != latest)
        err = "Only the most recent activity that is not yet reversed (No. " +
              to_string(latest->data.sequence) + ") can be undone first.";
    else if (applyReversal(record, err))
    {
        record.undone = true;
        record.undoneTime = currentTimestamp();
        save();
        if (record.action == "REGISTER")
            message = "Activity " + number + " reversed: resource " + record.resourceId +
                      " has been removed.";
        else
            message = "Activity " + number + " reversed: " + record.resourceId + " restored from " +
                      record.newStatus + " to " + record.previousStatus + ".";
        return true;
    }
    return false; // err was set by one of the branches above
}

// Undo the newest activity that has not been reversed.
bool ActivityManager::undoLatest(string &message, string &err)
{
    if (history.isEmpty())
    {
        err = "No activity history to undo.";
        return false;
    }
    ActivityNode *latest = findLatestActive();
    if (latest == nullptr)
    {
        err = "Every recorded activity has already been reversed.";
        return false;
    }
    return reverseActivity(latest, message, err);
}

// Undo a chosen activity number (only allowed when it is the newest not-yet-reversed one).
bool ActivityManager::undoBySequence(int sequence, string &message, string &err)
{
    if (history.isEmpty())
    {
        err = "No activity history to undo.";
        return false;
    }
    ActivityNode *node = findBySequence(sequence);
    if (node == nullptr)
    {
        err = "Activity No. " + to_string(sequence) + " does not exist.";
        return false;
    }
    return reverseActivity(node, message, err);
}

// =====================================================================
//  6. Shared instance + Task 3 hook
// =====================================================================
ActivityManager &getActivityManager()
{
    static ActivityManager manager("data/activities.txt");
    return manager;
}

// Registers the ONE hook Task 3 allows. After this, every register / status /
// assign / release / interrupt / remove in Task 3 is recorded automatically.
void connectActivityLogging()
{
    getResourceManager().setActivityLogger(
        [](const string &action, const string &details)
        { getActivityManager().record(action, details); });
}

// =====================================================================
//  7. Menu (one small function per option)
// =====================================================================
namespace
{
// Prints the success text or "Error: ..." for an operation result.
void reportActivityResult(bool success, const string &successText, const string &err)
{
    cout << "\n  " << (success ? successText : "Error: " + err) << "\n";
}

// ---- option 1 ----
void recentFlow(const ActivityManager &manager)
{
    int howMany = readInt(" How many recent activities to show (1-50): ", 1, 50);
    if (cin.eof())
        return;
    manager.displayRecent(howMany);
}

// ---- option 2 ----
void historyForFlow(const ActivityManager &manager)
{
    string id = readLine("\n Parcel/Request/Resource ID: ");
    if (cin.eof())
        return;
    manager.displayHistoryFor(id);
}

// ---- option 4 ----
void undoLastFlow(ActivityManager &manager)
{
    if (manager.count() == 0)
    {
        cout << "\n  No activity history to undo.\n";
        return;
    }
    if (!manager.displayUndoCandidate())
    {
        cout << "\n  Every recorded activity has already been reversed.\n";
        return;
    }
    if (!readYesNo(" Undo this activity?"))
    {
        cout << "  Cancelled.\n";
        return;
    }
    string message, err;
    bool ok = manager.undoLatest(message, err);
    reportActivityResult(ok, message, err);
}

// ---- option 5 ----
void undoSelectedFlow(ActivityManager &manager)
{
    if (manager.count() == 0)
    {
        cout << "\n  No activity history to undo.\n";
        return;
    }
    int number = readInt("\n Activity number to undo (0 = cancel): ", 0, 99999999);
    if (cin.eof() || number == 0)
        return;
    if (!manager.displayActivity(number))
    {
        cout << "\n  Error: Activity No. " << number << " does not exist.\n";
        return;
    }
    if (!readYesNo(" Undo this activity?"))
    {
        cout << "  Cancelled.\n";
        return;
    }
    string message, err;
    bool ok = manager.undoBySequence(number, message, err);
    reportActivityResult(ok, message, err);
}
} // namespace

// Menu entry point called by main.cpp
void activityMenu()
{
    ActivityManager &manager = getActivityManager();
    int choice = -1;
    do
    {
        cout << "\n--- ACTIVITY HISTORY ---\n";
        cout << " 1. Show recent activities\n";
        cout << " 2. Show history for a parcel/request/resource ID\n";
        cout << " 3. View most recent activity\n";
        cout << " 4. Undo last activity\n";
        cout << " 5. Undo a selected activity (by number)\n";
        cout << " 0. Back\n";
        cout << "----------------------------------------------\n";
        choice = readInt(" Enter choice: ", 0, 5);
        if (cin.eof())
            return;

        switch (choice)
        {
        case 1: recentFlow(manager); break;
        case 2: historyForFlow(manager); break;
        case 3: manager.displayMostRecent(); break;
        case 4: undoLastFlow(manager); break;
        case 5: undoSelectedFlow(manager); break;
        case 0: break;
        }
    } while (choice != 0);
}