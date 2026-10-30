// parcelQueue.cpp - linked list of Parcel records + linked FIFO queue of IDs
#include "parcelQueue.hpp"

// ====================== ParcelList ======================

ParcelList::ParcelList() : head(0), tail(0), count(0) {}

ParcelList::~ParcelList() { clear(); }

void ParcelList::clear()
{
    while (head != 0)
    {
        ParcelListNode *doomed = head;
        head = head->next;
        delete doomed;
    }
    tail  = 0;
    count = 0;
}

// Insert at the tail so records stay in registration order. O(1).
void ParcelList::append(const Parcel &p)
{
    ParcelListNode *node = new ParcelListNode;
    node->data = p;
    node->next = 0;

    if (tail == 0)          // empty list
        head = tail = node;
    else
    {
        tail->next = node;
        tail = node;
    }
    count++;
}

Parcel *ParcelList::findById(const std::string &id) const
{
    for (ParcelListNode *cur = head; cur != 0; cur = cur->next)
        if (cur->data.parcelID == id)
            return &cur->data;
    return 0;
}

// Unlink one node, handling head / middle / tail cases. O(n).
bool ParcelList::removeById(const std::string &id)
{
    ParcelListNode *prev = 0;
    ParcelListNode *cur  = head;

    while (cur != 0 && cur->data.parcelID != id)
    {
        prev = cur;
        cur  = cur->next;
    }
    if (cur == 0)
        return false;                       // not found

    if (prev == 0)  head = cur->next;       // removing the head
    else            prev->next = cur->next;
    if (cur == tail) tail = prev;           // removing the tail

    delete cur;
    count--;
    return true;
}

// ====================== ParcelQueue ======================

ParcelQueue::ParcelQueue() : frontNode(0), rearNode(0), count(0) {}

ParcelQueue::~ParcelQueue() { clear(); }

void ParcelQueue::clear()
{
    std::string dummy;
    while (dequeue(dummy)) {}
}

void ParcelQueue::enqueue(const std::string &id)
{
    ParcelQueueNode *node = new ParcelQueueNode;
    node->parcelId = id;
    node->next     = 0;

    if (rearNode == 0)
        frontNode = rearNode = node;
    else
    {
        rearNode->next = node;
        rearNode = node;
    }
    count++;
}

bool ParcelQueue::dequeue(std::string &idOut)
{
    if (frontNode == 0)
        return false;                       // underflow: empty queue

    ParcelQueueNode *old = frontNode;
    idOut     = old->parcelId;
    frontNode = old->next;
    if (frontNode == 0)
        rearNode = 0;                       // queue became empty
    delete old;
    count--;
    return true;
}

bool ParcelQueue::peek(std::string &idOut) const
{
    if (frontNode == 0)
        return false;
    idOut = frontNode->parcelId;
    return true;
}

bool ParcelQueue::contains(const std::string &id) const
{
    for (ParcelQueueNode *cur = frontNode; cur != 0; cur = cur->next)
        if (cur->parcelId == id)
            return true;
    return false;
}

// Remove an arbitrary ID while keeping the order of the others.
bool ParcelQueue::removeId(const std::string &id)
{
    ParcelQueueNode *prev = 0;
    ParcelQueueNode *cur  = frontNode;

    while (cur != 0 && cur->parcelId != id)
    {
        prev = cur;
        cur  = cur->next;
    }
    if (cur == 0)
        return false;

    if (prev == 0)  frontNode = cur->next;
    else            prev->next = cur->next;
    if (cur == rearNode) rearNode = prev;

    delete cur;
    count--;
    return true;
}