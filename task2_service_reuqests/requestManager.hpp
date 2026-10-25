#pragma once

#include <string>

#include "../common/Utils.hpp"
#include "../common/serviceRequest.hpp"
#include "requestPriorityQueue.hpp"

// These nodes form the singly linked list that stores every request.
struct RequestNode
{
    ServiceRequest request;
    RequestNode *next;

    explicit RequestNode(const ServiceRequest &newRequest);
};

class RequestManager
{
private:
    RequestNode *head;
    RequestNode *tail;
    int requestCount;
    std::string filePath;
    RequestPriorityQueue priorityQueue;
    common::ActivityLogger logger;

    RequestNode *findNode(const std::string &requestID) const;
    bool parcelIsRegistered(const std::string &parcelID) const;
    bool save(std::string &error) const;
    void logActivity(const std::string &action, const std::string &details) const;
    std::string makeRequestID() const;

public:
    explicit RequestManager(const std::string &path = "data/requests.txt");
    ~RequestManager();

    RequestManager(const RequestManager &) = delete;
    RequestManager &operator=(const RequestManager &) = delete;

    void setActivityLogger(common::ActivityLogger activityLogger);
    bool load(std::string &error);

    bool createRequest(ServiceRequest &request, std::string &error);
    void displayActiveRequests() const;
    void displayFinishedRequests() const;
    bool displayRequest(const std::string &requestID) const;
    bool displayNextRequest() const;
    bool processNextRequest(std::string &error);
    bool cancelRequest(const std::string &requestID, std::string &error);
    bool finishProcessingRequest(const std::string &requestID,
                                 const std::string &finalStatus,
                                 std::string &error);

    static bool isValidServiceType(const std::string &serviceType);
    static bool isValidStatus(const std::string &status);
};

void requestMenu();