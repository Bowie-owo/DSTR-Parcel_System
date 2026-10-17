// =====================================================================
//  task3_resources/resourceCircularQueue.hpp
//  Hand-written CIRCULAR QUEUE of resource IDs (no STL container used).
//
//  Storage : a raw dynamic array of fixed capacity.
//  Indexing: the front index wraps around the end of the array with the
//            modulo operator, so freed slots at the front are reused.
//
//  Complexity: enqueue, dequeue, peek, isEmpty, isFull, size are all O(1).
//  Space     : O(capacity).
//
//  ResourceManager keeps one queue per resource type (counters, desks,
//  special-handling counters, riders) to give out work in round-robin order.
// =====================================================================
#pragma once

#include <string>

#include "../common/Resource.hpp"

class ResourceCircularQueue
{
private:
    std::string *slots; // the underlying array
    int capacity;       // maximum number of IDs
    int frontIndex;     // index of the oldest ID
    int itemCount;      // number of IDs currently stored

public:
    explicit ResourceCircularQueue(int maxItems = resourceDefs::MAX_PER_TYPE);
    ~ResourceCircularQueue();

    // Copying would make two queues share one array, so it is disabled.
    ResourceCircularQueue(const ResourceCircularQueue &) = delete;
    ResourceCircularQueue &operator=(const ResourceCircularQueue &) = delete;

    bool isEmpty() const { return itemCount == 0; }
    bool isFull() const { return itemCount == capacity; }
    int size() const { return itemCount; }

    bool enqueue(const std::string &resourceId);    // false when full
    bool dequeue(std::string &resourceId);          // false when empty
    bool peek(std::string &resourceId) const;       // front item, not removed
    void clear();                                   // empty the queue
};