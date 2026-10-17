// =====================================================================
//  task3_resources/resourceCircularQueue.cpp
//  Implementation of the hand-written circular queue.
// =====================================================================
#include "resourceCircularQueue.hpp"

using namespace std;

ResourceCircularQueue::ResourceCircularQueue(int maxItems)
    : slots(nullptr), capacity(maxItems < 1 ? 1 : maxItems), frontIndex(0), itemCount(0)
{
    slots = new string[capacity];
}

ResourceCircularQueue::~ResourceCircularQueue()
{
    delete[] slots;
}

// Adds an ID at the back. The back index wraps around the array end.
bool ResourceCircularQueue::enqueue(const string &resourceId)
{
    if (isFull())
        return false;
    int backIndex = (frontIndex + itemCount) % capacity;
    slots[backIndex] = resourceId;
    itemCount++;
    return true;
}

// Removes the front ID. The front index wraps around the array end.
bool ResourceCircularQueue::dequeue(string &resourceId)
{
    if (isEmpty())
        return false;
    resourceId = slots[frontIndex];
    frontIndex = (frontIndex + 1) % capacity;
    itemCount--;
    return true;
}

bool ResourceCircularQueue::peek(string &resourceId) const
{
    if (isEmpty())
        return false;
    resourceId = slots[frontIndex];
    return true;
}

void ResourceCircularQueue::clear()
{
    frontIndex = 0;
    itemCount = 0;
}