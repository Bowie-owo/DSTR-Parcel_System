#pragma once

#include "../common/serviceRequest.hpp"

struct RequestNode;

// This queue is made from linked nodes, not the STL priority_queue.
// It always keeps the most important pending request at the front.
class RequestPriorityQueue
{
private:
    struct QueueNode
    {
        RequestNode *requestNode;
        QueueNode *next;

        explicit QueueNode(RequestNode *request);
    };

    QueueNode *front;

    static bool comesBefore(const ServiceRequest &first, const ServiceRequest &second);

public:
    RequestPriorityQueue();
    ~RequestPriorityQueue();

    RequestPriorityQueue(const RequestPriorityQueue &) = delete;
    RequestPriorityQueue &operator=(const RequestPriorityQueue &) = delete;

    bool isEmpty() const;
    RequestNode *peek() const;
    void enqueue(RequestNode *requestNode);
    RequestNode *dequeue();
    bool remove(RequestNode *requestNode);
    void clear();
};
