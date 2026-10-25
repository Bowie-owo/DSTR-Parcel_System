#include "requestManager.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace std;
using namespace common;

namespace
{
const string SERVICE_TYPES[] = {
    "Recipient Collection",
    "Campus Delivery",
    "Special Handling",
    "Return to Sender",
    "Re-delivery"};

const int SERVICE_TYPE_COUNT = 5;

bool isFinishedStatus(const string &status)
{
    return status == "Completed" || status == "Cancelled" || status == "Returned";
}

bool hasSafeText(const string &text, size_t maxLength)
{
    return !text.empty() && text.size() <= maxLength &&
           text.find('|') == string::npos && text.find_first_of("\r\n") == string::npos;
}

void printRequest(const ServiceRequest &request)
{
    cout << "Request ID:      " << request.requestID << "\n"
         << "Parcel ID:       " << request.parcelID << "\n"
         << "Recipient ID:    " << request.recipientID << "\n"
         << "Recipient Name:  " << request.recipientName << "\n"
         << "Service Type:    " << request.serviceType << "\n"
         << "Request Time:    " << request.requestDateTime << "\n"
         << "Status:          " << request.status << "\n";
}

bool parseRequest(const string &line, ServiceRequest &request)
{
    stringstream fields(line);
    string values[7];

    for (int i = 0; i < 7; ++i)
    {
        if (!getline(fields, values[i], '|'))
            return false;
        values[i] = trim(values[i]);
    }

    string extraField;
    if (getline(fields, extraField, '|'))
        return false;

    request.requestID = toUpper(values[0]);
    request.parcelID = toUpper(values[1]);
    request.recipientID = toUpper(values[2]);
    request.recipientName = values[3];
    request.serviceType = values[4];
    request.requestDateTime = values[5];
    request.status = values[6];

    return isValidId(request.requestID) &&
           isValidId(request.parcelID) &&
           isValidId(request.recipientID) &&
           hasSafeText(request.recipientName, 80) &&
           RequestManager::isValidServiceType(request.serviceType) &&
           hasSafeText(request.requestDateTime, 30) &&
           RequestManager::isValidStatus(request.status);
}
} // namespace

RequestNode::RequestNode(const ServiceRequest &newRequest)
    : request(newRequest), next(nullptr)
{
}

RequestManager::RequestManager(const string &path)
    : head(nullptr), tail(nullptr), requestCount(0), filePath(path)
{
    string error;
    if (!load(error))
        cerr << "Could not load service requests: " << error << "\n";
}

RequestManager::~RequestManager()
{
    RequestNode *current = head;
    while (current != nullptr)
    {
        RequestNode *next = current->next;
        delete current;
        current = next;
    }
}

void RequestManager::setActivityLogger(ActivityLogger activityLogger)
{
    logger = activityLogger;
}

void RequestManager::logActivity(const string &action, const string &details) const
{
    if (logger)
        logger(action, details);
}

bool RequestManager::isValidServiceType(const string &serviceType)
{
    for (int i = 0; i < SERVICE_TYPE_COUNT; ++i)
        if (serviceType == SERVICE_TYPES[i])
            return true;
    return false;
}

bool RequestManager::isValidStatus(const string &status)
{
    return status == "Pending" || status == "Processing" ||
           status == "Completed" || status == "Cancelled" || status == "Returned";
}

RequestNode *RequestManager::findNode(const string &requestID) const
{
    string searchID = toUpper(trim(requestID));
    RequestNode *current = head;
    while (current != nullptr)
    {
        if (current->request.requestID == searchID)
            return current;
        current = current->next;
    }
    return nullptr;
}

bool RequestManager::parcelIsRegistered(const string &parcelID) const
{
    ifstream file("data/parcels.txt");
    if (!file)
        return false;

    string line;
    while (getline(file, line))
    {
        size_t separator = line.find('|');
        string savedID = trim(line.substr(0, separator));
        if (toUpper(savedID) == toUpper(parcelID))
            return true;
    }
    return false;
}

string RequestManager::makeRequestID() const
{
    int highestNumber = 0;
    RequestNode *current = head;
    while (current != nullptr)
    {
        const string &id = current->request.requestID;
        if (id.size() > 3 && id.substr(0, 3) == "REQ")
        {
            string digits = id.substr(3);
            if (digits.find_first_not_of("0123456789") == string::npos)
            {
                int number = 0;
                stringstream(digits) >> number;
                if (number > highestNumber)
                    highestNumber = number;
            }
        }
        current = current->next;
    }

    while (highestNumber < 999999999)
    {
        ++highestNumber;
        ostringstream result;
        result << "REQ" << setw(3) << setfill('0') << highestNumber;
        if (findNode(result.str()) == nullptr)
            return result.str();
    }
    return "";
}

bool RequestManager::save(string &error) const
{
    makeParentDir(filePath);
    ofstream file(filePath.c_str(), ios::trunc);
    if (!file)
    {
        error = "Cannot open " + filePath + " for writing.";
        return false;
    }

    RequestNode *current = head;
    while (current != nullptr)
    {
        const ServiceRequest &request = current->request;
        file << request.requestID << '|'
             << request.parcelID << '|'
             << request.recipientID << '|'
             << request.recipientName << '|'
             << request.serviceType << '|'
             << request.requestDateTime << '|'
             << request.status << '\n';
        current = current->next;
    }

    if (!file)
    {
        error = "A write error occurred while saving " + filePath + ".";
        return false;
    }
    return true;
}

bool RequestManager::load(string &error)
{
    ifstream file(filePath.c_str());
    if (!file)
    {
        error = "Cannot open " + filePath + " for reading.";
        return false;
    }

    priorityQueue.clear();
    RequestNode *current = head;
    while (current != nullptr)
    {
        RequestNode *next = current->next;
        delete current;
        current = next;
    }
    head = nullptr;
    tail = nullptr;
    requestCount = 0;

    string line;
    int badLines = 0;
    while (getline(file, line))
    {
        if (trim(line).empty())
            continue;

        ServiceRequest request;
        if (!parseRequest(line, request) || findNode(request.requestID) != nullptr)
        {
            ++badLines;
            continue;
        }

        RequestNode *newNode = new RequestNode(request);
        if (tail == nullptr)
            head = newNode;
        else
            tail->next = newNode;
        tail = newNode;
        ++requestCount;

        if (request.status == "Pending")
            priorityQueue.enqueue(newNode);
    }

    if (badLines > 0)
    {
        ostringstream message;
        message << badLines << " invalid or duplicate request record(s) were skipped.";
        error = message.str();
        cerr << "Warning: " << error << "\n";
    }
    return true;
}

bool RequestManager::createRequest(ServiceRequest &request, string &error)
{
    request.parcelID = toUpper(trim(request.parcelID));
    request.recipientID = toUpper(trim(request.recipientID));
    request.recipientName = trim(request.recipientName);

    if (!isValidId(request.parcelID))
    {
        error = "Parcel ID is invalid.";
        return false;
    }
    if (!parcelIsRegistered(request.parcelID))
    {
        error = "That Parcel ID is not in data/parcels.txt. Register the parcel first.";
        return false;
    }
    if (!isValidId(request.recipientID))
    {
        error = "Recipient ID is invalid.";
        return false;
    }
    if (!hasSafeText(request.recipientName, 80))
    {
        error = "Recipient name must contain 1 to 80 characters and cannot contain |.";
        return false;
    }
    if (!isValidServiceType(request.serviceType))
    {
        error = "Choose one of the listed service types.";
        return false;
    }

    RequestNode *current = head;
    while (current != nullptr)
    {
        const ServiceRequest &saved = current->request;
        if (!isFinishedStatus(saved.status) &&
            saved.parcelID == request.parcelID &&
            saved.serviceType == request.serviceType)
        {
            error = "This parcel already has an active request for that service.";
            return false;
        }
        current = current->next;
    }

    request.requestID = makeRequestID();
    if (request.requestID.empty())
    {
        error = "No more request IDs are available.";
        return false;
    }
    request.requestDateTime = currentDateTime();
    if (request.requestDateTime.empty())
    {
        error = "The current date and time could not be read.";
        return false;
    }
    request.status = "Pending";

    RequestNode *newNode = new RequestNode(request);
    if (tail == nullptr)
        head = newNode;
    else
        tail->next = newNode;
    tail = newNode;
    ++requestCount;
    priorityQueue.enqueue(newNode);

    if (!save(error))
    {
        priorityQueue.remove(newNode);
        if (head == newNode)
        {
            head = nullptr;
            tail = nullptr;
        }
        else
        {
            RequestNode *previous = head;
            while (previous->next != newNode)
                previous = previous->next;
            previous->next = nullptr;
            tail = previous;
        }
        --requestCount;
        delete newNode;
        return false;
    }

    logActivity("CREATE_REQUEST", request.requestID + "|" + request.parcelID);
    return true;
}

void RequestManager::displayActiveRequests() const
{
    cout << "\n--- ACTIVE SERVICE REQUESTS ---\n";
    bool found = false;
    RequestNode *current = head;
    while (current != nullptr)
    {
        if (!isFinishedStatus(current->request.status))
        {
            printRequest(current->request);
            cout << "------------------------------\n";
            found = true;
        }
        current = current->next;
    }
    if (!found)
        cout << "There are no active requests.\n";
}

void RequestManager::displayFinishedRequests() const
{
    cout << "\n--- COMPLETED / CANCELLED / RETURNED REQUESTS ---\n";
    bool found = false;
    RequestNode *current = head;
    while (current != nullptr)
    {
        if (isFinishedStatus(current->request.status))
        {
            printRequest(current->request);
            cout << "------------------------------\n";
            found = true;
        }
        current = current->next;
    }
    if (!found)
        cout << "There are no finished requests.\n";
}

bool RequestManager::displayRequest(const string &requestID) const
{
    RequestNode *node = findNode(requestID);
    if (node == nullptr)
        return false;
    printRequest(node->request);
    return true;
}

bool RequestManager::displayNextRequest() const
{
    RequestNode *next = priorityQueue.peek();
    if (next == nullptr)
    {
        cout << "There are no pending requests to handle.\n";
        return false;
    }

    cout << "\n--- NEXT REQUEST TO HANDLE ---\n";
    printRequest(next->request);
    return true;
}

bool RequestManager::processNextRequest(string &error)
{
    RequestNode *next = priorityQueue.dequeue();
    if (next == nullptr)
    {
        error = "There are no pending requests to handle.";
        return false;
    }

    next->request.status = "Processing";
    if (!save(error))
    {
        next->request.status = "Pending";
        priorityQueue.enqueue(next);
        return false;
    }

    logActivity("PROCESS_REQUEST", next->request.requestID + "|" + next->request.parcelID);
    return true;
}

bool RequestManager::cancelRequest(const string &requestID, string &error)
{
    RequestNode *node = findNode(requestID);
    if (node == nullptr)
    {
        error = "Request ID was not found.";
        return false;
    }
    if (isFinishedStatus(node->request.status))
    {
        error = "A completed, cancelled or returned request cannot be cancelled again.";
        return false;
    }

    string oldStatus = node->request.status;
    if (oldStatus == "Pending")
        priorityQueue.remove(node);
    node->request.status = "Cancelled";
    if (!save(error))
    {
        node->request.status = oldStatus;
        if (oldStatus == "Pending")
            priorityQueue.enqueue(node);
        return false;
    }

    logActivity("CANCEL_REQUEST", node->request.requestID + "|" + node->request.parcelID);
    return true;
}

bool RequestManager::finishProcessingRequest(const string &requestID,
                                            const string &finalStatus,
                                            string &error)
{
    RequestNode *node = findNode(requestID);
    if (node == nullptr)
    {
        error = "Request ID was not found.";
        return false;
    }
    if (node->request.status != "Processing")
    {
        error = "Only a request with Processing status can be completed or returned.";
        return false;
    }
    if (finalStatus != "Completed" && finalStatus != "Returned")
    {
        error = "Final status must be Completed or Returned.";
        return false;
    }

    node->request.status = finalStatus;
    if (!save(error))
    {
        node->request.status = "Processing";
        return false;
    }

    logActivity(finalStatus == "Returned" ? "RETURN_REQUEST" : "COMPLETE_REQUEST",
                node->request.requestID + "|" + node->request.parcelID);
    return true;
}

void requestMenu()
{
    RequestManager requests;
    while (true)
    {
        cout << "\n--- SERVICE REQUESTS ---\n"
             << " 1. Create service request\n"
             << " 2. Display active requests\n"
             << " 3. Search request by ID\n"
             << " 4. View next request to handle\n"
             << " 5. Process next request\n"
             << " 6. Cancel request\n"
             << " 7. Complete or return a processing request\n"
             << " 8. Display completed/cancelled/returned requests\n"
             << " 9. Back\n";

        int choice = readInt(" Choose: ", 1, 9);
        if (cin.eof() || choice == 9)
            return;

        string error;
        if (choice == 1)
        {
            ServiceRequest request;
            request.parcelID = readLine(" Parcel ID: ");
            request.recipientID = readLine(" Recipient ID: ");
            request.recipientName = readLine(" Recipient name: ");
            request.serviceType = pickFromList(" Select service type", SERVICE_TYPES, SERVICE_TYPE_COUNT);
            if (request.serviceType.empty())
                return;

            if (requests.createRequest(request, error))
                cout << "Request created successfully. New Request ID: " << request.requestID << "\n";
            else
                cout << "Request was not created: " << error << "\n";
        }
        else if (choice == 2)
        {
            requests.displayActiveRequests();
        }
        else if (choice == 3)
        {
            string requestID = readLine(" Request ID: ");
            if (!requests.displayRequest(requestID))
                cout << "Request ID was not found.\n";
        }
        else if (choice == 4)
        {
            requests.displayNextRequest();
        }
        else if (choice == 5)
        {
            if (!requests.displayNextRequest())
                continue;
            if (!readYesNo("Are the recipient and service details correct?"))
            {
                cout << "Request left pending so its details can be checked.\n";
                continue;
            }
            if (requests.processNextRequest(error))
                cout << "Request is now Processing. Use option 7 when it is finished.\n";
            else
                cout << "Request was not processed: " << error << "\n";
        }
        else if (choice == 6)
        {
            string requestID = readLine(" Request ID to cancel: ");
            if (requests.cancelRequest(requestID, error))
                cout << "Request cancelled.\n";
            else
                cout << "Request was not cancelled: " << error << "\n";
        }
        else if (choice == 7)
        {
            string requestID = readLine(" Processing Request ID: ");
            cout << " 1. Completed\n 2. Returned to sender\n";
            int finalChoice = readInt(" Final status: ", 1, 2);
            if (cin.eof())
                return;
            string finalStatus = finalChoice == 1 ? "Completed" : "Returned";
            if (requests.finishProcessingRequest(requestID, finalStatus, error))
                cout << "Request status updated to " << finalStatus << ".\n";
            else
                cout << "Request was not updated: " << error << "\n";
        }
        else if (choice == 8)
        {
            requests.displayFinishedRequests();
        }
    }
}