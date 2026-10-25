#pragma once

#include <string>

// The details kept for one parcel service request.
struct ServiceRequest
{
    std::string requestID;
    std::string parcelID;
    std::string recipientID;
    std::string recipientName;
    std::string serviceType;
    std::string requestDateTime;
    std::string status;
};