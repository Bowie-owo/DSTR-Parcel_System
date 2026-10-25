#include "requestPriorityQueue.hpp"
#include "requestManager.hpp"

namespace
{
// Lower numbers are handled first.
int priorityFor(const std::string &serviceType)
{
    if (serviceType == "Special Handling")
        return 1;
    if (serviceType == "Return to Sender")
        return 2;
    if (serviceType == "Re-delivery")
        return 3;
    if (serviceType == "Campus Delivery")
        return 4;
    return 5; // Recipient Collection
}
} // namespace

RequestPriorityQueue::QueueNode::QueueNode(RequestNode *request)
    : requestNode(request), next(nullptr)
{
}

RequestPriorityQueue::RequestPriorityQueue() : front(nullptr)
{
}

RequestPriorityQueue::~RequestPriorityQueue()
{
    clear();
}

bool RequestPriorityQueue::comesBefore(const ServiceRequest &first, const ServiceRequest &second)
{
    int firstPriority = priorityFor(first.serviceType);
    int secondPriority = priorityFor(second.serviceType);

    if (firstPriority != secondPriority)
        return firstPriority < secondPriority;
    if (first.requestDateTime != second.requestDateTime)
        return first.requestDateTime < second.requestDateTime;
    return first.requestID < second.requestID;
}

bool RequestPriorityQueue::isEmpty() const
{
    return front == nullptr;
}

RequestNode *RequestPriorityQueue::peek() const
{
    return front == nullptr ? nullptr : front->requestNode;
}

void RequestPriorityQueue::enqueue(RequestNode *requestNode)
{
    if (requestNode == nullptr)
        return;

    QueueNode *newNode = new QueueNode(requestNode);
    if (front == nullptr || comesBefore(requestNode->request, front->requestNode->request))
    {
        newNode->next = front;
        front = newNode;
        return;
    }

    QueueNode *current = front;
    while (current->next != nullptr &&
           !comesBefore(requestNode->request, current->next->requestNode->request))
    {
        current = current->next;
    }

    newNode->next = current->next;
    current->next = newNode;
}

RequestNode *RequestPriorityQueue::dequeue()
{
    if (front == nullptr)
        return nullptr;

    QueueNode *oldFront = front;
    RequestNode *requestNode = oldFront->requestNode;
    front = front->next;
    delete oldFront;
    return requestNode;
}

bool RequestPriorityQueue::remove(RequestNode *requestNode)
{
    QueueNode *current = front;
    QueueNode *previous = nullptr;

    while (current != nullptr)
    {
        if (current->requestNode == requestNode)
        {
            if (previous == nullptr)
                front = current->next;
            else
                previous->next = current->next;

            delete current;
            return true;
        }

        previous = current;
        current = current->next;
    }
    return false;
}

void RequestPriorityQueue::clear()
{
    while (front != nullptr)
    {
        QueueNode *oldFront = front;
        front = front->next;
        delete oldFront;
    }
}
