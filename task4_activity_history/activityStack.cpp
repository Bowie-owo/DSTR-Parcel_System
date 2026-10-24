// =====================================================================
//  task4_activity_history/activityStack.cpp
//  Implementation of the hand-written linked stack.
// =====================================================================
#include "activityStack.hpp"

#include <new>

ActivityStack::ActivityStack() : topNode(nullptr), itemCount(0) {}

ActivityStack::~ActivityStack()
{
    clear();
}

// Adds a record on top of the stack. The new node points to the old top.
bool ActivityStack::push(const ActivityRecord &record)
{
    ActivityNode *newNode = new (std::nothrow) ActivityNode;
    if (newNode == nullptr) // out of memory: the only way a linked stack is "full"
        return false;
    newNode->data = record;
    newNode->next = topNode;
    topNode = newNode;
    itemCount++;
    return true;
}

// Removes the top record and hands it back. Fails (false) when the stack is empty.
bool ActivityStack::pop(ActivityRecord &record)
{
    if (isEmpty())
        return false;
    ActivityNode *oldTop = topNode;
    record = oldTop->data;
    topNode = oldTop->next;
    delete oldTop;
    itemCount--;
    return true;
}

// Copies the top record without removing it.
bool ActivityStack::peek(ActivityRecord &record) const
{
    if (isEmpty())
        return false;
    record = topNode->data;
    return true;
}

// Deletes every node (used by the destructor and when reloading from file).
void ActivityStack::clear()
{
    while (topNode != nullptr)
    {
        ActivityNode *oldTop = topNode;
        topNode = topNode->next;
        delete oldTop;
    }
    itemCount = 0;
}