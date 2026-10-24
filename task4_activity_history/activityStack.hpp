// =====================================================================
//  Task 4 - Parcel Activity and Record Management Module
//  task4_activity_history/activityStack.hpp
//
//  Hand-written STACK (LIFO) of activity records, built as a singly
//  linked list (no STL container used).
//
//  Why a stack?
//    Staff want to see the NEWEST activity first and to undo the MOST
//    RECENT operation first. Last-in-first-out gives both naturally:
//    the top of the stack is always the newest activity.
//
//  Why a linked list underneath (instead of an array)?
//    The number of activities is unknown and keeps growing during a
//    busy day. A linked stack never "overflows" at a fixed size and
//    push/pop never has to shift or copy other elements.
//
//  Complexity:  push, pop, peek, isEmpty, size  -> O(1)
//               traversal / search by ID        -> O(n)
//  Space:       O(n), one node per activity.
//
//  Note: when an activity is reversed it is NOT popped. It stays in the
//  stack with undone = true so the history is never lost (staff can
//  still see that it happened and when it was reversed).
// =====================================================================
#pragma once

#include <string>

// ---------------------------------------------------------------------
//  One recorded activity (the data held in every stack node).
//  Text fields use "-" when a value does not apply.
// ---------------------------------------------------------------------
struct ActivityRecord
{
    int sequence;               // running number: 1, 2, 3 ... (processing order)
    std::string timestamp;      // when it happened, "YYYY-MM-DD HH:MM"
    std::string action;         // REGISTER, STATUS, ASSIGN, RELEASE, ...
    std::string itemId;         // parcel or request ID involved
    std::string resourceId;     // counter / rider involved
    std::string previousStatus; // status before the activity
    std::string newStatus;      // status after the activity
    std::string details;        // raw text sent by the module that logged it
    bool undone;                // true once the activity has been reversed
    std::string undoneTime;     // when it was reversed ("-" if not reversed)

    ActivityRecord()
        : sequence(0), timestamp("-"), action("-"), itemId("-"), resourceId("-"),
          previousStatus("-"), newStatus("-"), details("-"), undone(false), undoneTime("-")
    {
    }
};

// One node of the linked stack.
struct ActivityNode
{
    ActivityRecord data;
    ActivityNode *next; // the node BELOW this one (older activity)
};

class ActivityStack
{
private:
    ActivityNode *topNode; // newest activity (nullptr when empty)
    int itemCount;         // number of activities stored

public:
    ActivityStack();
    ~ActivityStack();

    // Copying would make two stacks share the same nodes, so it is disabled.
    ActivityStack(const ActivityStack &) = delete;
    ActivityStack &operator=(const ActivityStack &) = delete;

    bool isEmpty() const { return topNode == nullptr; }
    int size() const { return itemCount; }

    bool push(const ActivityRecord &record); // false only if memory runs out
    bool pop(ActivityRecord &record);        // false when empty
    bool peek(ActivityRecord &record) const; // copy of the top, not removed

    // Read access to the newest node so callers can walk down the stack
    // (newest -> oldest) without removing anything. nullptr when empty.
    ActivityNode *top() const { return topNode; }

    void clear(); // delete every node
};