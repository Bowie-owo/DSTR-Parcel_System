#ifndef PARCEL_HPP
#define PARCEL_HPP

#include <string>

// Enum representing the status of a parcel
enum class ParcelStatus {
    REGISTERED,
    SORTED,
    PROCESSING,
    READY_FOR_COLLECTION,
    OUT_FOR_DELIVERY,
    COMPLETED,
    RETURNED,
    MISSING,
    CANCELLED
};

// Convert status enum to string for display purposes
inline std::string parcelStatusToString(ParcelStatus status) {
    switch (status) {
        case ParcelStatus::REGISTERED: return "Registered";
        case ParcelStatus::SORTED: return "Sorted";
        case ParcelStatus::PROCESSING: return "Processing";
        case ParcelStatus::READY_FOR_COLLECTION: return "Ready for Collection";
        case ParcelStatus::OUT_FOR_DELIVERY: return "Out for Delivery";
        case ParcelStatus::COMPLETED: return "Completed";
        case ParcelStatus::RETURNED: return "Returned";
        case ParcelStatus::MISSING: return "Missing";
        case ParcelStatus::CANCELLED: return "Cancelled";
    }
    return "Unknown";
}

// Core Parcel structure shared across modules
struct Parcel {
    std::string parcelID;
    std::string recipientID;
    std::string recipientName;
    std::string parcelType;
    std::string arrivalDateTime;
    std::string status;          // Can store string or use enum status
    std::string deliveryInfo;    // Additional notes or location
};

#endif