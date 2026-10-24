// =====================================================================
//  Task 4 - Parcel Activity and Record Management Module
//  task4_activity_history/activityManager.hpp
//
//  Files used by this module:
//    ../common/Utils.hpp              readInt, readYesNo, readLine, ... (shared)
//    ../task3_resources/resourceManager.hpp   used to REVERSE resource activities
//    activityStack.hpp/.cpp           hand-written linked stack of activities
//    activityManager.hpp/.cpp         this class + activityMenu()
//
//  Data structure (no STL containers):
//    - ActivityStack : newest activity on top, used for history and undo
//
//  What this module does
//    1. RECORD   activities coming from other modules
//                  - Task 3 (resources) is connected through the hook
//                    getResourceManager().setActivityLogger(...)
//                  - Task 1 / Task 2 call recordActivity() directly
//    2. DISPLAY  recent activities, the history of one parcel/request/
//                resource ID, and the most recent activity
//    3. UNDO     the most recent activity that is allowed to be reversed
//                (currently REGISTER and STATUS, see isReversible)
//    4. SAVE     the history to data/activities.txt so it survives restarts
//
//  Undo rule (LIFO): only the NEWEST activity that has not yet been
//  reversed can be undone. The stack enforces this order, so a parcel/
//  request/resource is never rolled back past a later change.
// =====================================================================
#pragma once

#include <string>

#include "activityStack.hpp"

class ActivityManager
{
private:
    ActivityStack history;    // newest activity on top
    int nextSequence;         // number given to the next recorded activity
    bool undoing;             // true while an undo is running (see record())
    std::string filePath;     // data file for saving the history

    // Recording helper: stamps sequence + time, pushes on the stack, saves.
    void addRecord(ActivityRecord &record);

    // Searching the stack (walks from newest to oldest).
    ActivityNode *findLatestActive() const;           // newest NOT reversed
    ActivityNode *findBySequence(int sequence) const; // exact activity number

    // Undo helpers.
    bool reverseActivity(ActivityNode *node, std::string &message, std::string &err);
    bool applyReversal(const ActivityRecord &record, std::string &err);
    static bool isReversible(const std::string &action);

public:
    explicit ActivityManager(const std::string &path = "data/activities.txt");

    // File handling
    bool load();
    bool save() const;

    // ---- Requirement 1: record activities ---------------------------
    // Hook entry used by Task 3. 'details' is the text Task 3 sends, e.g.
    //   REGISTER  "CC01|Collection Counter"
    //   STATUS    "CC01|Available|Maintenance"
    //   ASSIGN    "CC01|PCL001"
    // Ignored while an undo is running (so an undo is not logged as a new activity).
    void record(const std::string &action, const std::string &details);

    // Full version for Task 1 / Task 2. Use "-" for any value that does not apply.
    //   recordActivity("REGISTER_PARCEL", "PCL001", "-", "-", "Registered", "Parcel registered");
    void recordActivity(const std::string &action, const std::string &itemId,
                        const std::string &resourceId, const std::string &previousStatus,
                        const std::string &newStatus, const std::string &details);

    // ---- Requirement 2 and 3: display and review --------------------
    void displayRecent(int maxItems) const;                 // newest first
    void displayHistoryFor(const std::string &id) const;    // oldest first (order they happened)
    void displayMostRecent() const;                         // newest activity in full detail
    bool displayUndoCandidate() const;                      // newest NOT reversed; false if none
    bool displayActivity(int sequence) const;               // one activity in full detail

    // ---- Requirement 4 and 5: reverse an activity -------------------
    bool undoLatest(std::string &message, std::string &err);
    bool undoBySequence(int sequence, std::string &message, std::string &err);

    int count() const { return history.size(); }
};

// Shared instance so every module (Task 1/2/3) records into the same history.
ActivityManager &getActivityManager();

// Call ONCE at program start (main.cpp). Registers the single Task 3 hook so
// every resource action is recorded automatically.
void connectActivityLogging();

// Menu entry point expected by main.cpp
void activityMenu();