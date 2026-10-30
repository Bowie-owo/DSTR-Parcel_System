// =====================================================================
//  parcelQueue.hpp - Task 1 data structures
//   1. ParcelList  : SINGLY LINKED LIST - the master file of all parcels
//   2. ParcelQueue : LINKED FIFO QUEUE  - parcels waiting to be processed
//
//  Why a linked list for the records?
//   - the number of parcels is unknown and grows during peak periods,
//     so no fixed array size / no costly resizing is needed
//   - append at the tail is O(1) (tail pointer is kept)
//   - removing a record only re-links pointers (no shifting of elements)
//   - search by ID is O(n) (acceptable for a prototype; Task 5's tree
//     gives the O(log n) lookup)
//
//  Why a queue for the waiting parcels?
//   - parcels must be processed in arrival order (first come, first
//     served), which is exactly FIFO
//   - enqueue (rear) and dequeue (front) are both O(1)
//   - only the ID is stored, so the master record lives in one place
//     (ParcelList) and cannot become inconsistent
//   - linked, so it never becomes "full" during peak periods
// =====================================================================
#ifndef PARCEL_INTAKE_QUEUE_HPP
#define PARCEL_INTAKE_QUEUE_HPP

#include <string>
#include "../common/Parcel.hpp"

// ---------------------------------------------------------------------
//  ParcelList - singly linked list of Parcel records
// ---------------------------------------------------------------------
struct ParcelListNode
{
    Parcel          data;
    ParcelListNode *next;
};

class ParcelList
{
private:
    ParcelListNode *head;
    ParcelListNode *tail;
    int             count;

    ParcelList(const ParcelList &);            // copying disabled (owns raw nodes)
    ParcelList &operator=(const ParcelList &);

public:
    ParcelList();
    ~ParcelList();

    void            append(const Parcel &p);               // O(1)
    Parcel         *findById(const std::string &id) const; // O(n)
    bool            removeById(const std::string &id);     // O(n)
    void            clear();

    bool            isEmpty() const { return count == 0; }
    int             size() const    { return count; }
    ParcelListNode *first() const   { return head; }       // for traversal
};

// ---------------------------------------------------------------------
//  ParcelQueue - linked FIFO queue of Parcel IDs
// ---------------------------------------------------------------------
struct ParcelQueueNode
{
    std::string      parcelId;
    ParcelQueueNode *next;
};

class ParcelQueue
{
private:
    ParcelQueueNode *frontNode;
    ParcelQueueNode *rearNode;
    int              count;

    ParcelQueue(const ParcelQueue &);
    ParcelQueue &operator=(const ParcelQueue &);

public:
    ParcelQueue();
    ~ParcelQueue();

    void  enqueue(const std::string &id);          // O(1)
    bool  dequeue(std::string &idOut);             // O(1), false if empty
    bool  peek(std::string &idOut) const;          // O(1), false if empty
    bool  contains(const std::string &id) const;   // O(n)
    bool  removeId(const std::string &id);         // O(n) (cancel / hold / missing)
    void  clear();

    bool  isEmpty() const { return count == 0; }
    int   size() const    { return count; }
    const ParcelQueueNode *front() const { return frontNode; } // for display
};

#endif